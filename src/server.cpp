// ==============================================================================
// Copyright 2020 The LatticeX Foundation
// This file is part of the Rosetta library.
//
// The Rosetta library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Rosetta library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with the Rosetta library. If not, see <http://www.gnu.org/licenses/>.
// ==============================================================================
#include "io/internal/server.h"
#include <chrono>
#include <iostream>
#include <errno.h>
#include <algorithm>
using namespace std;
using namespace std::chrono;

namespace rosetta {
namespace io {

std::mutex TCPServer::connections_mtx_;
std::map<string, shared_ptr<Connection>> TCPServer::connections_; // client id --> connection
std::vector<shared_ptr<Connection>> TCPServer::recycle_connections_;
int TCPServer::task_count_ = 0;
std::mutex TCPServer::task_mtx_;
std::condition_variable TCPServer::task_cv_;
bool TCPServer::stoped_ = true;
Connection* TCPServer::listen_conn_ = nullptr;
int TCPServer::epollfd_ = -1;
int TCPServer::listenfd_ = -1;
int TCPServer::port_ = -1;
bool TCPServer::is_inited_ = false;
std::mutex TCPServer::init_mutex_;
int TCPServer::listen_count_ = 0;
std::mutex TCPServer::listen_mutex_;
std::condition_variable TCPServer::listen_cv_;

shared_ptr<Connection> TCPServer::get_connection(const string& node_id) {
  std::unique_lock<std::mutex> lck(server_connection_mtx_);
  auto iter = server_connections_.find(node_id);
  if (iter != server_connections_.end()) {
    return iter->second;
  }
  return nullptr;
}

bool TCPServer::put_connection(const string& node_id) {
  shared_ptr<Connection> conn = find_connection(node_id);
  if (conn != nullptr) {
    if (conn->is_reuseable()) {
      std::unique_lock<std::mutex> lck(server_connection_mtx_);
      server_connections_.insert(std::pair<string, shared_ptr<Connection>>(node_id, conn));
      return true;
    }
  }
  return false;
}
shared_ptr<Connection> TCPServer::find_connection(const string& cid) {
  unique_lock<mutex> lck(connections_mtx_);
  auto iter = connections_.find(cid);
  if (iter == connections_.end() || !iter->second->is_reuseable()) {
    return nullptr;
  }
  iter->second->start(task_id_);
  return iter->second;
}
uint64_t TCPServer::get_unrecv_size() {
  unique_lock<mutex> lck(connections_mtx_);
  uint64_t ret = 0;
  for (auto iter = connections_.begin(); iter != connections_.end(); iter++) {
    ret += iter->second->get_unrecv_size();
  }
  return ret;
}

//#define EPOLL_EVENTS (EPOLLIN | EPOLLERR)
#define EPOLL_EVENTS (EPOLLIN | EPOLLERR | EPOLLET)

void handleInterrupt(int sig) { cout << "Ctrl C" << endl; }
namespace {

void epoll_add(int efd, Connection* conn) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = conn->events_;
  ev.data.ptr = conn;
  int r = epoll_ctl(efd, EPOLL_CTL_ADD, conn->fd_, &ev);
  if (r != 0) {
    log_error << "epoll_ctl add failed. errno:" << errno << " " << strerror(errno) ;
  }
}

void epoll_mod(int efd, Connection* conn) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = conn->events_;
  ev.data.ptr = conn;
  int r = epoll_ctl(efd, EPOLL_CTL_MOD, conn->fd_, &ev);
  if (r != 0) {
    log_error << "epoll_ctl mod failed. errno:" << errno << " " << strerror(errno) ;
  }
}

void epoll_del(int efd, Connection* conn) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = conn->events_;
  ev.data.ptr = conn;
  int r = epoll_ctl(efd, EPOLL_CTL_DEL, conn->fd_, &ev);
  if (r != 0) {
    log_error << "epoll_ctl del failed. errno:" << errno << " " << strerror(errno) ;
  }
}
} // namespace

int TCPServer::create_server(int port) {
  int ret = -1;
  int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    log_error << "create socket failed. errno:" << errno << " " << strerror(errno) ;
    return -1;
  }

  set_nonblocking(fd, 1);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  // set options
  ret = set_reuseaddr(fd, option_reuseaddr() ? 1 : 0);
  ret = set_reuseport(fd, option_reuseport() ? 1 : 0);

  set_sendbuf(fd, default_buffer_size());
  set_recvbuf(fd, default_buffer_size());

  set_linger(fd);
  set_nodelay(fd, 1);

  ret = ::bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
  if (ret != 0) {
    log_error << "bind to 0.0.0.0:" << port << " failed. errno:" << errno << " " << strerror(errno)
              ;
    return -1;
  }

  ret = ::listen(fd, 20);
  if (ret != 0) {
    log_error << "listen failed. errno:" << errno << " " << strerror(errno) ;
    return -1;
  }

  return fd;
}

void TCPServer::handle_accept(Connection* conn) {
  struct sockaddr_in clt_addr;
  memset(&clt_addr, 0, sizeof(clt_addr));
  socklen_t clientlen = sizeof(clt_addr);
  int cfd = accept(listenfd_, (struct sockaddr*)&clt_addr, &clientlen);
  if (cfd == -1) {
    log_error << "accept failed. errno:" << errno << " " << strerror(errno) ;
    throw socket_exp("accept failed");
  }
  
  // send ack
  char ack = '1';
  ssize_t ret = ::write(cfd, &ack, sizeof(char));
  if (ret != sizeof(char)) {
    log_error << "write cid len error " << " ret:" << ret << " expected:" << sizeof(char);
    close(cfd);
    return;
  }

  uint64_t cid_len = -9999;
  // recv client id from client
  ret = ::read(cfd, (char*)&cid_len, sizeof(uint64_t));
  if (ret != sizeof(uint64_t)) {
    log_error << "read cid error" << "ret: " << ret << " expected:" << sizeof(uint64_t) ;
    close(cfd);
    return;
  }
  log_debug << "cid len:" << cid_len ;
  string cid;
  cid.resize(cid_len - sizeof(uint64_t));
  ret = ::read(cfd, &cid[0], cid_len - sizeof(uint64_t));
  if (ret != cid_len - sizeof(uint64_t)) {
    log_error << "read cid error " << "ret: " << ret << " expected:" << cid_len - sizeof(uint64_t) << "cid_len:" << cid_len;
    close(cfd);
    return;
  }

  if (std::find(expected_cids_.begin(), expected_cids_.end(), cid) == expected_cids_.end()) {
    log_warn << "not really client. cid:" << cid ;
    close(cfd);
    return;
  }

  log_audit << "recv node id:" << cid << " len:" << cid_len;
  log_debug << "server accept from client cid:" << cid;

  set_sendbuf(cfd, default_buffer_size());
  set_recvbuf(cfd, default_buffer_size());
  set_nodelay(cfd, 1);

  Connection* tc = nullptr;
  if (is_ssl_socket_)
    tc = new SSLConnection(cfd, EPOLL_EVENTS, true, cid);
  else
    tc = new Connection(cfd, EPOLL_EVENTS, true, cid);

  tc->ctx_ = ctx_;

  set_nonblocking(cfd, true);
  {
    unique_lock<mutex> lck(connections_mtx_);
    auto iter = connections_.find(cid);
    if (iter != connections_.end()) {
      log_warn << "recycle connection, node id:" << iter->second->node_id_;
      recycle_connections_.push_back(iter->second);
      connections_.erase(iter);
    }
    connections_.insert(std::pair<string, shared_ptr<Connection>>(cid, shared_ptr<Connection>(tc)));
    log_info << "server create connection ok " << cid;
  }
  epoll_add(epollfd_, tc);
  epoll_mod(epollfd_, listen_conn_);
}

void TCPServer::add_connection_to_epoll(shared_ptr<Connection> conn) {
  conn->ctx_ = ctx_;
  set_nonblocking(conn->fd_, true);
  conn->events_ = EPOLL_EVENTS;
  epoll_add(epollfd_, conn.get());
}

void TCPServer::handle_error(Connection* conn) {
  log_debug << __FUNCTION__ << " fd:" << conn->fd_ << " errno:" << errno << " " << strerror(errno);
  if (handler != nullptr) {
    handler("", conn->node_id_.c_str(), errno, strerror(errno), nullptr);
  }
  {
    //! double check
    struct tcp_info info;
    int len = sizeof(info);
    getsockopt(conn->fd_, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len);
    log_debug << __FUNCTION__ << " fd:" << conn->fd_
              << " tcp_info.tcpi_state: " << to_string(info.tcpi_state) ;
    if (info.tcpi_state == TCP_CLOSE) {
      conn->set_reuseable(false);
      while (true) {
        ssize_t len = conn->readImpl(conn->fd_, main_buffer_, 8192);
        if (len > 0) { // Normal
          conn->write(main_buffer_, len);
        } else {
          break;
        }
      }

      epoll_del(epollfd_, conn);
      conn->close(task_id_);
    }
  }
}

void TCPServer::handle_write(Connection* conn) { cout << __FUNCTION__ << endl; }

void TCPServer::handle_read(Connection* conn) {
  if (conn->fd_ == listenfd_) {
    handle_accept(conn);
    return;
  }

  if ((conn->state_ == Connection::State::Closing) || (conn->state_ == Connection::State::Closed)) {
    log_debug << "Closing or Closed." ;
    return;
  }

  // handle data read
  if (!conn->handshake()) {
    log_error << "server hadshake error" ;
    return;
  }

  while (true) {
    ssize_t len = conn->readImpl(conn->fd_, main_buffer_, 8192);
    if (len > 0) { // Normal
      conn->write(main_buffer_, len);
    } else if (len == 0) { // EOF
      log_error << "connection close";

      epoll_del(epollfd_, conn);
      conn->close(task_id_);
      return;
    } else { // <0, Error or Interrupt
      if (errno == EINTR) {
        continue;
      }
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        break;
      }
      break;
    }
  }
}

int timeout_counter = 0;

void TCPServer::loop_once(int efd, int waitms) {
  const int kMaxEvents = 20;
  struct epoll_event activeEvs[kMaxEvents];
  int nfds = epoll_wait(efd, activeEvs, kMaxEvents, waitms);
  if (nfds == 0) {
    timeout_counter++;
    return;
  } else if (nfds < 0) {
    log_error << "epoll error!" ;
    return;
  }

  timeout_counter = 0;
  for (int i = 0; i < nfds; i++) {
    Connection* conn = (Connection*)activeEvs[i].data.ptr;
    int events = activeEvs[i].events;

    if (events & EPOLLERR) {
      handle_error(conn);
    } else if (events & EPOLLIN) {
      handle_read(conn);
    } else if (events & EPOLLOUT) {
      handle_write(conn);
    } else {
      log_error << "unknown events " << events ;
    }
  }
}

void TCPServer::loop() {
  loop_thread_.join();
}

void TCPServer::loop_main() {
  // wait until no thread of other  tasks handles epoll events or this task finishes.
  {
    std::unique_lock<std::mutex> lck(listen_mutex_);
    listen_cv_.wait(lck, [&](){
    if (listen_count_ == 0 || stop_) {
      return true;
    }
    return false;
    });
    if (stop_) {
      return;
    }
    listen_count_++;
  }
  log_info << task_id_ << " begin loop epoll";
  int64_t timeout = -1;
  if (timeout < 0)
    timeout = 1000 * 1000000;
  timeout += wait_timeout_;
  int64_t elapsed = 0;
  auto beg = system_clock::now();

  /**
   * check if all clients have connected to this server
   * if timeout, break
   * if all has connected, break
   */
  bool all_has_connected_to_server = true;
  while (!stop_ && (elapsed <= timeout)) {
    all_has_connected_to_server = true;
    loop_once(epollfd_, 1000);
    {
      unique_lock<mutex> lck(connections_mtx_);
      for (int i = 0; i < expected_cids_.size(); i++) {
        auto iter = connections_.find(expected_cids_[i]);
        if (iter == connections_.end()) {
          all_has_connected_to_server = false;
          break;
        }
      }
    }
    if (all_has_connected_to_server)
      break;
    auto end = system_clock::now();
    elapsed = duration_cast<duration<int64_t, std::milli>>(end - beg).count();
    all_has_connected_to_server = false;
    log_debug << "server .... timeout:" << timeout << " elapsed:" << elapsed ;
  }

  if (all_has_connected_to_server) {
    while (!stop_) {
      loop_once(epollfd_, 1000);
    }
  } else {
    log_info << "client(s) connect to this server timeout, wait for closing..." ;
  }

  // notify the listen thread of other tasks to handler epoll events
  {
    std::unique_lock<std::mutex> lck(listen_mutex_);
    listen_count_--;
    listen_cv_.notify_one();
  }
  log_info << task_id_ << " end loop epoll";
}

bool TCPServer::init() {
  // 0
  //signal(SIGINT, handleInterrupt);

  // 1
  if ((epollfd_ = epoll_create1(EPOLL_CLOEXEC)) < 0) {
    log_error << "epoll_create1 failed. errno:" << errno << " " << strerror(errno) ;
    return false;
  }

  // 2
  if ((listenfd_ = create_server(port_)) < 0) {
    return false;
  }

  // 3
  if (is_ssl_socket_)
    listen_conn_ = new SSLConnection(listenfd_, EPOLL_EVENTS, true, "listen");
  else
    listen_conn_ = new Connection(listenfd_, EPOLL_EVENTS, true, "listen");

  // 4
  epoll_add(epollfd_, listen_conn_);

  return true;
}

bool TCPServer::start(const string& task_id, int port, error_callback handler_, int64_t timeout) {
  task_id_ = task_id;
  std::unique_lock<std::mutex> lck(task_mtx_);
  task_count_++;

  if (!is_inited_) {
    std::unique_lock<std::mutex> lck(init_mutex_);
    if (!is_inited_) {
      port_ = port;
      handler = handler_;
      if (!init_ssl())
        return false;
    
      if (!init())
        return false;
    
      stoped_ = false;
      is_inited_ = true;
      log_debug << "set stop false";
    }
  }

  loop_thread_ = thread(&TCPServer::loop_main, this);
  return true;
}

bool TCPServer::stop() {
  if (stop_)
    return true;
  log_info << task_id_ << " begin stop server";
  stop_ = 1;
  {
    std::unique_lock<std::mutex> lck(listen_mutex_);
    listen_cv_.notify_all();
  }
  loop_thread_.join();

  log_debug << task_id_ << "set stop true";

  for (auto iter = server_connections_.begin(); iter != server_connections_.end(); ) {
    iter->second->stop(task_id_);
    server_connections_.erase(iter++);
  }
   
   // the last task close listen fd and epoll fd
  {
    std::unique_lock<std::mutex> lck(task_mtx_);
    task_count_--;
    if (task_count_ == 0 && get_unrecv_size() == 0) {
      std::unique_lock<std::mutex> lck(connections_mtx_);
      for (auto& c : connections_) {
        if (c.second != nullptr) {
          if (c.second->state_ != Connection::State::Closed) {
            c.second->close(task_id_);
          }
        }
      }
      connections_.clear();

      for (auto iter = recycle_connections_.begin(); iter != recycle_connections_.end(); ) {
          iter = recycle_connections_.erase(iter);
      }
      // 6
      delete listen_conn_;
      listen_conn_ = nullptr;
      ::close(listenfd_);

      ::close(epollfd_);
      is_inited_ = false;
      stoped_ = true;
      log_debug << "server stopped!" ;
    }
  }
  log_info << task_id_ << " end stop server";
  return true;
}

} // namespace io
} // namespace rosetta
