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
#include "io/internal/connection.h"
#include "io/internal/simple_buffer.h"

#include <thread>
#include <chrono>
using namespace std::chrono;

namespace rosetta {
namespace io {

Connection::Connection(int _fd, int _events, bool _is_server, const string& node_id) {
  fd_ = _fd;
  events_ = _events;
  is_server_ = _is_server;
  node_id_ = node_id;
  buffer_ = make_shared<cycle_buffer>(1024 * 1024 * 10);
  send_buffer_ = make_shared<cycle_buffer>(1024 * 1024 * 128);
}

Connection::~Connection() { }

void Connection::close(const string& task_id) {
  if (state_ != Connection::State::Closed) {
    state_ = Connection::State::Closing;
    flush_send_buffer();
    ::close(fd_);
    state_ = Connection::State::Closed;
    log_info << task_id << " close connection ok " << node_id_ << " send buffer size:" << send_buffer_->size();
  }
}

SSLConnection::~SSLConnection() { close(); }

ssize_t Connection::send(const char* data, size_t len, int64_t timeout) {
  if (len > 1024 * 1024 * 100) {
    log_warn << "client will send " << len << " B, >100M!" ;
  }

  ssize_t n = 0;
  n = writen(fd_, data, len);
  return n;
}

ssize_t Connection::put_into_send_buffer(const char* data, size_t len, int64_t timeout) {
  std::unique_lock<std::mutex> lck(send_buffer_mtx_);
  ssize_t ret = send_buffer_->write(data, len);
  send_buffer_cv_.notify_all();
  return ret;
}

ssize_t Connection::send(const string& id, const char* data, uint64_t length, int64_t timeout) {
  simple_buffer buffer(id, data, length, node_id_);

  //log_debug << node_id_ << " send buffer:" << id << " len:" << buffer.len();
  return put_into_send_buffer((const char*)buffer.data(), buffer.len(), timeout);
}

uint64_t Connection::get_unrecv_size() {
  uint64_t ret = buffer_->size();
  {
    unique_lock<mutex> lck(mapbuffer_mtx_);
    for (auto iter = mapbuffer_.begin(); iter != mapbuffer_.end(); iter++) {
      ret += iter->second->size();
    }
  }
  return ret;
}

void Connection::loop_recv(string task_id) {
  log_debug << task_id << " begin loop recv data from " << node_id_;
  while (true) {
    
    string tmp_id;
    string tmp_data;
    {
      bool stop_recv = false;
      std::unique_lock<std::mutex> lck(buffer_mtx_);
      buffer_cv_.wait(lck, [&](){
        std::unique_lock<std::mutex> lck2(stop_work_mtx_);
        auto iter = stop_works_.find(task_id);
        if (iter != stop_works_.end() && iter->second) {
          stop_recv = true;
          return true;
        }
        if (buffer_->can_read()) {
          return true;
        }
        return false;
      });
      if (stop_recv) {
        break;
      }
      buffer_->read(tmp_id, tmp_data, node_id_);
    }

    {
      std::unique_lock<std::mutex> lck(mapbuffer_mtx_);
      if (mapbuffer_.find(tmp_id) == mapbuffer_.end()) {
        mapbuffer_[tmp_id] = make_shared<cycle_buffer>(1024 * 8);
      }
      // write the real data
      mapbuffer_[tmp_id]->write(tmp_data.data(), tmp_data.size());
      //log_debug << node_id_ << " write to mapbuffer, id:" << tmp_id << " size:" << tmp_data.size();
      mapbuffer_cv_.notify_all();
    }
  }
  log_debug << task_id << " end loop recv data from " << node_id_;
}

void Connection::flush_send_buffer() {
  uint64_t remain_size = send_buffer_->size();
  if (remain_size > 0) {
    char* buffer = new char[remain_size];
    send_buffer_->read(buffer, remain_size);
    ssize_t ret = send(buffer, remain_size);
    if (ret != remain_size) {
      log_error << "send data to " << node_id_ << " error, " << errno << ", error msg:" << strerror(errno);
    }
    log_debug << "send data to " << node_id_ << " size:" << ret;
    delete []buffer;
  }
}

void Connection::loop_send(string task_id) {
  log_debug << task_id << " begin loop send data to " << node_id_;
  while (true) {
    
    char *buffer = nullptr;
    uint64_t n = 0;
    {
      bool stop_send = false;
      std::unique_lock<std::mutex> lck(send_buffer_mtx_);
      send_buffer_cv_.wait(lck, [&](){
        std::unique_lock<std::mutex> lck2(stop_work_mtx_);
        auto iter = stop_works_.find(task_id);
        if (iter != stop_works_.end() && iter->second) {
          stop_send = true;
          return true;
        }
        if (send_buffer_->size() > 0) {
          return true;
        }
        return false;
      });
      if (stop_send) {
        break;
      }
      n = send_buffer_->size();
      buffer = new char[n];
      send_buffer_->read(buffer, n);
    }
    {
      ssize_t ret = send(buffer, n);
      if (ret != n) {
        log_error << "send data to " << node_id_ << " error, " << errno << ", error msg:" << strerror(errno);
      }
      log_debug << "send data to " << node_id_ << " size:" << ret;
      delete []buffer;
      //if (stop_send)return;
    }
  }
  log_debug << task_id << " end loop send data to " << node_id_;
}

void Connection::do_start(const string& task_id) {
  {
    std::unique_lock<std::mutex> lck(stop_work_mtx_);
    stop_works_.insert(std::pair<string, bool>(task_id, false));
    stop_work_cv_.notify_all();
  }
  bool stop_work = false;
  {
    std::unique_lock<std::mutex> lck(work_mtx_);
    work_cv_.wait(lck, [&](){
      if (work_count_ == 0) {
        return true;
      }
      std::unique_lock<std::mutex> lck2(stop_work_mtx_);
      auto iter = stop_works_.find(task_id);
      if (iter != stop_works_.end() && iter->second) {
        stop_work = true;
        return true;
      }
      return false;
    });
    if (stop_work) {
      return;
    }
    work_count_++;
    work_task_id_ = task_id;
  }
  std::thread recv_thread = thread(&Connection::loop_recv, this, task_id);
  std::thread send_thread = thread(&Connection::loop_send, this, task_id);
  recv_thread.join();
  send_thread.join();
  {
    std::unique_lock<std::mutex> lck(work_mtx_);
    work_count_--;
    work_cv_.notify_all();
  }
}

void Connection::do_stop(const string& task_id) {
  {
    {
      std::unique_lock<std::mutex> lck(stop_work_mtx_);
      auto iter = stop_works_.find(task_id);
      if (iter != stop_works_.end() && !iter->second) {
        iter->second = true;
      }
    }

    std::unique_lock<std::mutex> lck(work_mtx_);
    if (work_task_id_ == task_id) {
      {
        std::unique_lock<std::mutex> lck(send_buffer_mtx_);
        send_buffer_cv_.notify_all();
      }
      {
        std::unique_lock<std::mutex> lck(buffer_mtx_);
        buffer_cv_.notify_all();
      }
    } else {
      work_cv_.notify_all();
    }
  }
  
  std::thread *start_thread = nullptr;
  {
    std::unique_lock<std::mutex> lck(thread_mtx_);
    auto iter = threads_.find(task_id);
    if (iter != threads_.end()) {
      start_thread = iter->second;
      threads_.erase(iter);
    }
  }
  start_thread->join();
  delete start_thread;
  {
    std::unique_lock<std::mutex> lck(stop_work_mtx_);
    auto iter = stop_works_.find(task_id);
    if (iter != stop_works_.end()) {
      stop_works_.erase(iter);
    }
  }
}

void Connection::start(const string& task_id) {
  {
    std::unique_lock<std::mutex> lck(task_mtx_);
    task_count_++;
    string id = "lock:" + task_id;
    string msg = "1";
    send(id, msg.data(), msg.size(), -1);
  }
  std::thread *start_thread = new std::thread();
  *start_thread = thread(&Connection::do_start, this, task_id);
  {
    std::unique_lock<std::mutex> lck(thread_mtx_);
    threads_.insert(std::pair<string, std::thread*>(task_id, start_thread));
  }
  {
    std::unique_lock<std::mutex> lck(stop_work_mtx_);
    stop_work_cv_.wait(lck, [&](){
      auto iter = stop_works_.find(task_id);
      if (iter != stop_works_.end() && !iter->second) {
        return true;
      }
      return false;
    });
  }
}

void Connection::stop(const string& task_id) {
  log_info << task_id << " begin stop connection with " << node_id_;
  {
    std::unique_lock<std::mutex> lck(task_mtx_);
    task_count_--;

    string id = "lock:" + task_id;
    string msg = "1";
    recv(id, &msg[0], msg.size(), -1);
  }
  do_stop(task_id);
  log_info << task_id << " end stop connection with " << node_id_;
}

ssize_t Connection::recv(const string& id, char* data, uint64_t length, int64_t timeout) {
  if (timeout < 0)
    timeout = 1000 * 1000000;

  int64_t elapsed = 0;
  auto beg = system_clock::now();
  ssize_t ret = 0;
  bool retry = false;
  State tmpstate = state_;
  do {
    auto end = system_clock::now();
    elapsed = duration_cast<duration<int64_t, std::milli>>(end - beg).count();
    if ((tmpstate == State::Connected) && (state_ != State::Connected)) {
      return E_UNCONNECTED; // un connected
    }
    if (elapsed > timeout) {
      return E_TIMEOUT; // timeout
    }
    
    shared_ptr<cycle_buffer> buffer = nullptr;
    unique_lock<mutex> lck(mapbuffer_mtx_);
    mapbuffer_cv_.wait(lck, [&](){
      auto iter = mapbuffer_.find(id);
      if (iter != mapbuffer_.end()) { // got id
        buffer = iter->second;
        if (buffer->can_read(length)) { // got data
          return true;
        }
      }
      //log_debug << node_id_ << " not find mapbuffer, begin wait "<< id;
      return false;
    });
    ret = buffer->read(data, length);
    mapbuffer_cv_.notify_all();
    //log_debug << node_id_ << " read mapbuffer, notify all " << id;
    return ret;
  } while (true);

  log_error << "in fact, never enter here" ;
  return E_ERROR;
}

ssize_t Connection::peek(int sockfd, void* buf, size_t len) {
  while (1) {
    ssize_t ret = ::recv(sockfd, buf, len, MSG_PEEK);
    if (ret == -1 && errno == EINTR)
      continue;
    return ret;
  }
}

ssize_t Connection::readn(int connfd, char* vptr, size_t n) {
  ssize_t nleft;
  ssize_t nread;
  char* ptr;

  ptr = vptr;
  nleft = n;

  while (nleft > 0) {
    if ((nread = readImpl(connfd, ptr, nleft)) < 0) {
      if (errno == EINTR) {
        nread = 0;
      } else {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
          continue;
        }
        log_error << __FUNCTION__ << " errno:" << errno << " " << strerror(errno) ;
        return -1;
      }
    } else if (nread == 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      break;
    }
    nleft -= nread;
    ptr += nread;
  }
  return n - nleft;
}

ssize_t Connection::writen(int connfd, const char* vptr, size_t n) {
  ssize_t nleft, nwritten;
  const char* ptr;

  std::unique_lock<mutex> lck(mtx_send_);

  ptr = vptr;
  nleft = n;

  while (nleft > 0) {
    if ((nwritten = writeImpl(connfd, ptr, nleft)) < 0) {
      if (errno == EINTR) {
        nwritten = 0;
      } else if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      log_error << __FUNCTION__ << " errno:" << errno << " " << strerror(errno) ;
      return -1;
    } else if (nwritten == 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      break;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }

  return n - nleft;
}

void SSLConnection::close() {
  state_ = Connection::State::Closing;
  if (ssl_ != nullptr) {
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    ssl_ = nullptr;
  }
  ::close(fd_);
  state_ = Connection::State::Closed;
}

bool SSLConnection::handshake() {
  if (state_ == State::Connected) {
    return true;
  }
  if (ssl_ == nullptr) {
    {
      // new ssl
      ssl_ = SSL_new(ctx_);
      if (ssl_ == nullptr) {
        ERR_print_errors_fp(stderr);
        log_error << "SSLConnection::handshake() SSL_new failed!" ;
        return false;
      }
    }
    {
      // set fd to ssl
      int r = SSL_set_fd(ssl_, fd_);
      if (is_server()) {
        SSL_set_accept_state(ssl_);
      } else {
        SSL_set_connect_state(ssl_);
      }
    }

    // do handshake
    int fail_retries = 0;
    int r = -2;
    do {
#if 0
      if (is_server()) {
        r = SSL_accept(ssl_);
      } else {
        r = SSL_connect(ssl_);
      }
#else
      r = SSL_do_handshake(ssl_);
#endif
      if (r == 1) {
        state_ = State::Connected;
        // DEBUG INFO
        // const SSL_CIPHER* sc = SSL_get_current_cipher(ssl_);
        // log_info << "SSL VERSION INFO - CIPHER  VERSION:" << SSL_CIPHER_get_version(sc);
        // log_info << "SSL VERSION INFO - CIPHER     NAME:" << SSL_CIPHER_get_name(sc);
        // log_info << "SSL VERSION INFO - CIPHER STD NAME:" << SSL_CIPHER_standard_name(sc);
        // int alg_bits;
        // log_info << "SSL VERSION INFO - CIPHER ALG BITS:" << SSL_CIPHER_get_bits(sc, &alg_bits);
        return show_certs(ssl_, "");
      }

      int err = SSL_get_error(ssl_, r);
      if ((err == SSL_ERROR_WANT_WRITE) || (err == SSL_ERROR_WANT_READ)) {
        std::this_thread::sleep_for(chrono::milliseconds(100));
        continue;
      } else {
        log_warn << "is server:" << is_server() << " SSL_do_handshake retries:" << fail_retries
                 << " sslerr " << err << ":" << errno << " " << strerror(errno) ;
        if (fail_retries-- <= 0) {
          return false;
        }
      }
      std::this_thread::sleep_for(chrono::milliseconds(1000));
    } while (true);
  }

  if (SSL_is_init_finished(ssl_)) {
    return true;
  }
  return false;
} // namespace io

ssize_t SSLConnection::readImpl(int fd, char* data, size_t len) {
  size_t rd = 0;
#if 1
  {
    unique_lock<std::mutex> lck(ssl_rw_mtx_);
    int ret = SSL_read(ssl_, data, len);
    rd = ret;
    int e = SSL_get_error(ssl_, ret);
    //cout << "readImpl ssl ret:" << ret << ",r:" << rd << ", errno:" << errno << ", e:" << e << endl;
    if (ret < 0) {
      if ((e == SSL_ERROR_WANT_READ) || (e == SSL_ERROR_WANT_WRITE)) {
        return ret;
      }
      ERR_print_errors_fp(stdout);
      log_error << "SSLConnection::readImpl sslerr:" << e << ", errno:" << errno << " "
                << strerror(errno) ;
      return ret;
    }
  }
#else
  {
    //usleep(10000);
    //cout << "......................readImpl" << endl;
    unique_lock<std::mutex> lck(ssl_rw_mtx_);
    int ret = SSL_read_ex(ssl_, data, len, &rd);
    int e = SSL_get_error(ssl_, ret);
    //cout << "readImpl ssl ret:" << ret << ",r:" << rd << ", errno:" << errno << ", e:" << e << endl;
    if (ret < 0 && e != SSL_ERROR_WANT_READ) {
      ERR_print_errors_fp(stdout);
      log_error << "SSLConnection::readImpl errno:" << errno << " " << strerror(errno) << ",e:" << e
                ;
      throw socket_exp("SSLConnection::readImpl failed!");
    }
  }
#endif
  return rd;
}
ssize_t SSLConnection::writeImpl(int fd, const char* data, size_t len) {
  size_t wr = 0;
#if 1
  {
    unique_lock<std::mutex> lck(ssl_rw_mtx_);
    int ret = SSL_write(ssl_, data, len);
    wr = ret;
    int e = SSL_get_error(ssl_, ret);
    //cout << "wriImpl ssl ret:" << ret << ",r:" << wr << ", errno:" << errno << ", e:" << e << endl;
    if (ret < 0) {
      if ((e == SSL_ERROR_WANT_READ) || (e == SSL_ERROR_WANT_WRITE)) {
        return ret;
      }
      ERR_print_errors_fp(stdout);
      log_error << "SSLConnection::writeImpl sslerr:" << e << ", errno:" << errno << " "
                << strerror(errno) ;
      return ret;
    }
  }
#else
  {
    //usleep(100);
    //cout << "......................writeImpl" << endl;
    unique_lock<std::mutex> lck(ssl_rw_mtx_);
    int ret = SSL_write_ex(ssl_, data, len, &wr);
    int e = SSL_get_error(ssl_, ret);
    //cout << "wriImpl ssl ret:" << ret << ",r:" << wr << ", errno:" << errno << ", e:" << e << endl;
    if (ret < 0 && e != SSL_ERROR_WANT_WRITE) {
      ERR_print_errors_fp(stdout);
      log_error << "SSLConnection::writeImpl errno:" << errno << ",e:" << e ;
      throw socket_exp("SSLConnection::writeImpl failed!");
    }
  }
#endif
  return wr;
}

} // namespace io
} // namespace rosetta
