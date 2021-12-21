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
#include "io/internal/client.h"
#include <chrono>
#include <thread>
using namespace std::chrono;
#define NEVER_TIMEOUT 1000 * 1000000

namespace rosetta {
namespace io {
map<string, shared_ptr<Connection>> TCPClient::connections_;
vector<shared_ptr<Connection>> TCPClient::recycle_connections_;
set<string> TCPClient::to_connect_set_;

int TCPClient::task_count_ = 0;
std::mutex TCPClient::task_mtx_;
std::condition_variable TCPClient::task_cv_;

bool TCPClient::connect(int64_t timeout, int64_t conn_retries) {
  if (!init_ssl())
    return false;

  ip_ = Socket::gethostip(host_);
  if (ip_ == "") {
    log_error << "Can not get right IP by " << host_ ;
    return false;
  }

  log_debug << "client[" << cid_ << "] is ready to connect to server[" << ip_ << ":" << port_ << "]";
  string node_address = ip_ + ":" + std::to_string(port_);
  {
    std::unique_lock<std::mutex> lck(task_mtx_);
    if (task_count_ == 0) {
      is_first_client_ = true;
    }
    task_count_++;

    auto iter = connections_.find(node_address);
    if (iter != connections_.end()) {
      if (iter->second->is_reuseable()) {
        log_debug << "find connection " << node_address;
        conn_ = iter->second;
        conn_->start(task_id_);
        connected_ = true;
        return true;
      } else {
        recycle_connections_.push_back(iter->second);
        connections_.erase(iter);
      }
    }
    auto iter2 = to_connect_set_.find(node_address);
    if (iter2 != to_connect_set_.end()) {
      task_cv_.wait(lck, [&](){
        auto iter3 = connections_.find(node_address);
        if (iter3 != connections_.end()) {
          return true;
        }
        return false;
      });
      iter = connections_.find(node_address);
      if (iter != connections_.end()) {
        log_debug << "find connection " << node_address;
        conn_ = iter->second;
        conn_->start(task_id_);
        connected_ = true;
        return true;
      }
    }
    to_connect_set_.insert(node_address);
  }

  ///////////////////////////////////////////
  //! @todo retries 3 times default
  if (conn_retries <= 0)
    conn_retries = 1;
  for (int k = 0; k < conn_retries; k++) {
    struct sockaddr_in server;

    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
      log_warn << "client create socket failed" ;
      continue;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_.c_str());
    server.sin_port = htons(port_);

    int err = -1;

    set_sendbuf(fd_, default_buffer_size());
    set_recvbuf(fd_, default_buffer_size());
    set_nodelay(fd_, 1);
    set_linger(fd_);

    if (timeout < 0)
      timeout = NEVER_TIMEOUT; // 100w s

    //! in vm, connect api is not in blocking, even under blocking mode!!!
    int64_t elapsed = 0;
    auto beg = system_clock::now();
    bool connected = false;
    while ((timeout < 0) || (elapsed <= timeout)) {
      if (timeout > 0) {
        int64_t tout = timeout - elapsed;
        set_send_timeout(fd_, tout);
        set_recv_timeout(fd_, tout);
      } else {
        break;
      }

      err = ::connect(fd_, (struct sockaddr*)&server, sizeof(server));
      if (err == 0) {
        log_debug << "client connect to server ok " << node_address;
        connected = true;
        break;
      }
      // #define	EISCONN		106	/* Transport endpoint is already connected */
      if (errno == EISCONN) {
        log_error << "already connected:" << errno << " " << strerror(errno);
        connected = true;
        break;
      }

      // #define	ECONNREFUSED	111	/* Connection refused */
      if (errno == ECONNREFUSED) {
        //! continue;
      }
      std::this_thread::sleep_for(chrono::milliseconds(500));
      auto end = system_clock::now();
      elapsed = duration_cast<duration<int64_t, std::milli>>(end - beg).count();
    }

    if (!connected) {
      if (elapsed > timeout) {
        log_warn << "client[" << cid_ << "] connect to server[" << ip_ << ":" << port_
                 << "] timeout." ;
        ::close(fd_);
        continue;
      }
      log_error << "client[" << cid_ << "] connect to server[" << ip_ << ":" << port_ << "] failed."
                ;
      ::close(fd_);
      continue;
    }

    // recv ack
    auto beg_read = system_clock::now();
    char connect_ack;
    ssize_t ret = ::read(fd_, (char*)&connect_ack, sizeof(char));
    if (ret != sizeof(char)) {
      if (ret != 0) {
        log_error << "client recv ack error. ret:" << ret << ", errno:" << errno << ", strerror:" <<  strerror(errno);
      }
      ::close(fd_);

      auto end_read = system_clock::now();
      auto read_elapsed = duration_cast<duration<int64_t, std::milli>>(end_read - beg_read).count();
      auto sleep_time = k < conn_retries - 1 ? timeout - read_elapsed : 0;
      log_error << "read ack from " << node_id_ << " failed, sleep " << (sleep_time > 0 ? sleep_time : 0) << "ms, retries:" << (k + 1);
      if (sleep_time > 0) {
        std::this_thread::sleep_for(chrono::milliseconds(sleep_time));
      }
      continue;
    }

    string tmpcid;
    uint64_t cid_len = sizeof(uint64_t) + cid_.size();
    log_audit << "send node id:" << cid_ << " len:" << cid_len;
    tmpcid.resize(cid_len);
    memcpy(&tmpcid[0], &cid_len, sizeof(uint64_t));
    memcpy((char*)&tmpcid[0] + sizeof(uint64_t), cid_.data(), cid_.size());
    ret = ::write(fd_, (const char*)&tmpcid[0], cid_len);
    if (ret != cid_len) {
      log_error << "client send cid error. ret:" << ret << ", errno:" << errno << " , strerror:" << strerror(errno);
      ::close(fd_);
      continue;
    }

    set_send_timeout(fd_, NEVER_TIMEOUT);
    set_recv_timeout(fd_, NEVER_TIMEOUT);

    if (is_ssl_socket_)
      conn_ = std::make_shared<SSLConnection>(fd_, 0, false, node_id_);
    else
      conn_ = std::make_shared<Connection>(fd_, 0, false, node_id_);
    conn_->ctx_ = ctx_;
    connected_ = true;

    set_nonblocking(fd_, true);
    if (conn_->handshake()) {
      {
        std::unique_lock<std::mutex> lck(task_mtx_);
        connections_.insert(std::pair<string, shared_ptr<Connection>>(node_address, conn_));
        auto iter = to_connect_set_.find(node_address);
        if (iter != to_connect_set_.end()) {
          to_connect_set_.erase(iter);
        }
        task_cv_.notify_all();
      }
      is_first_connect_ = true;
      conn_->start(task_id_);
      log_info << "client create connection ok " << node_address;
      return true;
    }

    int64_t interval = 1000;
    if (timeout > elapsed) {
      interval += timeout - elapsed;
    }
    if (interval > 5000) {
      interval = 5000;
    }
    log_warn << "client[" << cid_ << "] will retry connect to server[" << ip_ << ":" << port_
             << "]. retries:" << k << ", at " << interval << "(ms) later." ;
    std::this_thread::sleep_for(chrono::milliseconds(interval));
  }
  ///////////////////////////////////////////
  return false;
}

uint64_t TCPClient::get_unrecv_size() {
  uint64_t ret = 0;
  for (auto iter = connections_.begin(); iter != connections_.end(); iter++) {
    ret += iter->second->get_unrecv_size();
  }
  return ret;
}

void TCPClient::close() {
  if (connected_) {
    connected_ = false;
    conn_->stop(task_id_);
    {
      std::unique_lock<std::mutex> lck(task_mtx_);
      task_count_--;

      //[todo] the connection should not stop when the peer node has task reusing the connection link
      // for example, P1 and P2 are two nodes, P1 is client and P2 is server.
      // firstly, P1 start task T1 and create a connection C1 to P2, P2 start task T2 and accept connection from P1,
      // the two tasks started on P1 and P2 are blocked as they are different tasks.
      // secondly, P2 start task T1 and finishes T1 with P1. P1 will close the connection as no other task
      // uses this connection. P2 will keep the connection as T2 still use the connection.
      // thirdly, P1 start task T2 and create a new connection C2 to P2 as there is no connection available.
      // now the problem comes in.P1 send/recv data on connection C2 while P2 send/recv data on connection C1.
      if (task_count_ == 0 && get_unrecv_size() == 0) {
        for (auto iter = connections_.begin(); iter != connections_.end(); ) {
          iter->second->close(task_id_);
          connections_.erase(iter++);
        }
        for (auto iter = recycle_connections_.begin(); iter != recycle_connections_.end(); ) {
          iter = recycle_connections_.erase(iter);
        }
        to_connect_set_.clear();
        log_debug << "client stopped";
      }
    }
  }
}

} // namespace io
} // namespace rosetta
