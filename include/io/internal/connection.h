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

#pragma once
#include "io/internal/cycle_buffer.h"
#include "io/internal/socket.h"
#include "io/internal/ssl_socket.h"

#include <atomic>
#include <map>
#include <iostream>
#include <mutex>
#include <thread>
using namespace std;

namespace rosetta {
namespace io {

struct Connection {
 public:
  Connection(int _fd, int _events, bool _is_server, const string& node_id);
  virtual ~Connection();

 public:
  virtual bool handshake() {return true;}
  virtual void close(const string& task_id);
  bool is_server() const { return is_server_; }

 public:
  ssize_t send(const char* data, size_t len, int64_t timeout = -1L);
  ssize_t put_into_send_buffer(const char* data, size_t len, int64_t timeout = -1L);
  ssize_t send(const string& id, const char* data, uint64_t length, int64_t timeout = -1L);
  ssize_t recv(const string& id, char* data, uint64_t length, int64_t timeout = -1L);

  // Read & Write
 public:
  ssize_t peek(int sockfd, void* buf, size_t len);
  ssize_t readn(int connfd, char* vptr, size_t n);
  ssize_t writen(int connfd, const char* vptr, size_t n);
  void write(const char* data, size_t len) {
    buffer_->write(data, len);
    log_debug << "recv data from " << node_id_ << " size:" << len;
    std::unique_lock<std::mutex> lck(buffer_mtx_);
    buffer_cv_.notify_all();
  }

  virtual ssize_t readImpl(int fd, char* data, size_t len) {
    ssize_t ret = ::read(fd, data, len);
    return ret;
  }
  virtual ssize_t writeImpl(int fd, const char* data, size_t len) {
    ssize_t ret = ::write(fd, data, len);
    return ret;
  }

  void start(const string& task_id);
  void stop(const string& task_id);
  void set_reuseable(bool reuseable) {
    reuseable_ = reuseable;
  }
  bool is_reuseable() {
    return reuseable_;
  }
  uint64_t get_unrecv_size();

private:
  void start_recv();
  void stop_recv();
  void loop_recv(string task_id);
  void start_send();
  void stop_send();
  void loop_send(string task_id);
  void do_start(const string& task_id);
  void do_stop(const string& task_id);
  void flush_send_buffer();

 protected:
  std::mutex mtx_send_;
  std::atomic<int> atomic_send_{0};

 public:
  enum State {
    Invalid = 1,
    Handshaking,
    Handshaked,
    Connecting,
    Connected,
    Closing,
    Closed,
    Failed,
  };
  State state_ = State::Invalid;

  int fd_ = -1;
  int events_ = 0;
  bool is_server_ = false;
  string client_ip_ = "";
  int client_port_ = 0;
  string node_id_ = "";
  bool reuseable_ = true;

  //! buffer manage
  //! for all messages
  shared_ptr<cycle_buffer> buffer_ = nullptr;
  //! for one message which id is msg_id_t
  map<string, shared_ptr<cycle_buffer>> mapbuffer_;
  shared_ptr<cycle_buffer> send_buffer_ = nullptr;
  std::mutex mapbuffer_mtx_;
  std::mutex buffer_mtx_;
  std::mutex send_buffer_mtx_;
  std::condition_variable mapbuffer_cv_;
  std::condition_variable buffer_cv_;
  std::condition_variable send_buffer_cv_;

  map<string, bool> stop_works_;
  std::mutex stop_work_mtx_;
  std::condition_variable stop_work_cv_;

  int task_count_ = 0;
  std::mutex task_mtx_;
  std::condition_variable task_cv_;

  int work_count_ = 0;
  string work_task_id_ = "";
  std::mutex work_mtx_;
  std::condition_variable work_cv_;

  map<string, std::thread*> threads_;
  std::mutex thread_mtx_;

  SSL_CTX* ctx_ = nullptr; // do not delete this pointer in this class
};

class SSLConnection : public Connection {
  using Connection::Connection;
  SSL* ssl_ = nullptr;
  std::mutex ssl_rw_mtx_;

 public:
  ~SSLConnection();
  virtual void close();
  virtual bool handshake();
  virtual ssize_t readImpl(int fd, char* data, size_t len);
  virtual ssize_t writeImpl(int fd, const char* data, size_t len);
};

} // namespace io
} // namespace rosetta
