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

#include "io/internal/connection.h"
#include "io/internal/socket.h"
#include "io/internal/ssl_socket.h"
#include "io/channel.h"
#include <vector>
#include <thread>
using namespace std;

namespace rosetta {
namespace io {

class TCPServer : public Socket {
 public:
  TCPServer() { main_buffer_ = new char[1024 * 1024 * 2]; }
  virtual ~TCPServer() {
    stop();
    delete[] main_buffer_;
  }

 public:
  bool start(const string& task_id, int port, error_callback handler_, int64_t timeout = -1L);
  bool stop();
  bool stoped() { return stoped_; }
  void add_connection_to_epoll(shared_ptr<Connection> conn);
  void loop();

 public:

  /**
   * about certifications
   */
  virtual bool init_ssl() { return true; }
  void set_server_cert(string server_cert) { server_cert_ = server_cert; }
  void set_server_prikey(string server_prikey, string password = "") {
    server_prikey_ = server_prikey;
    server_prikey_password_ = password;
  }
  void setsid(const string& sid) { sid_ = sid; }
  void set_expected_cids(const vector<string>& expected_cids) { expected_cids_ = expected_cids; }
  void setwtimo(int64_t wait_timeout) { wait_timeout_ = wait_timeout; }
  bool put_connection(const string& node_id);
  shared_ptr<Connection> get_connection(const string& node_id);
  static uint64_t get_unrecv_size();

 protected:
  shared_ptr<Connection> find_connection(const string& cid);

 protected:
  bool init();
  int create_server(int port);
  void loop_once(int efd, int waitms);
  void loop_main();

  void handle_accept(Connection* conn);
  void handle_read(Connection* conn);
  void handle_write(Connection* conn);
  void handle_error(Connection* conn);

 protected:
  static Connection* listen_conn_;
  std::thread loop_thread_;
  static int epollfd_;
  static int listenfd_;
  static bool is_inited_;
  static std::mutex init_mutex_;
  static int listen_count_;
  static std::mutex listen_mutex_;
  static std::condition_variable listen_cv_;

 protected:
  SSL_CTX* ctx_ = nullptr;
  static std::mutex connections_mtx_;
  static std::map<string, shared_ptr<Connection>> connections_; // client id --> connection
  static std::vector<shared_ptr<Connection>> recycle_connections_;
  static int task_count_;
  static std::mutex task_mtx_;
  static std::condition_variable task_cv_;
  char* main_buffer_ = nullptr;
  static int port_;
  int stop_ = 0;
  static bool stoped_;
  string task_id_ = "";
  std::map<string, shared_ptr<Connection>> server_connections_;
  std::mutex server_connection_mtx_;

  string server_cert_;
  string server_prikey_;
  string server_prikey_password_;

  std::condition_variable init_cv_;
  std::mutex init_mtx_;

  string sid_ = ""; // server id
  vector<string> expected_cids_;
  int64_t wait_timeout_ = 0;
  error_callback handler = nullptr;
};

class SSLServer : public TCPServer {
  using TCPServer::TCPServer;

 public:
  virtual bool init_ssl();
  SSLServer();
  virtual ~SSLServer();
};

} // namespace io
} // namespace rosetta
