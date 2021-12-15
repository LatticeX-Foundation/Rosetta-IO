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
#include <set>

namespace rosetta {
namespace io {

class TCPClient : public Socket {
 public:
  TCPClient(const string& task_id, const string& node_id, const std::string& host, int port) 
    : task_id_(task_id), node_id_(node_id), host_(host), ip_(host), port_(port) {}
  ~TCPClient() { close(); }

 public:
  bool connected() const { return connected_; }
  void setcid(const string& cid) { cid_ = cid; }
  void setsid(const string& sid) { sid_ = sid; }
  void setsslid(const string& sslid) { sslid_ = sslid; }
  shared_ptr<Connection> get_connection() { return conn_; }
  static uint64_t get_unrecv_size();

 public:
  /**
   * \param timeout ms
   */
  bool connect(int64_t timeout, int64_t conn_retries);
  void close();
  bool closed() { return !connected_; }
  bool is_first_connect() { return is_first_connect_; }

  virtual bool init_ssl() { return true; }

 protected:
  string host_ = ""; // Domain www.xxxx.yyy
  string ip_ = ""; // IP xxx.xxx.xxx.xxx
  int port_ = 0;
  int fd_ = -1;
  string node_id_ = "";
  string task_id_ = "";
  bool connected_ = false;
  bool is_first_connect_ = false;
  shared_ptr<Connection> conn_ = nullptr;
  static map<string, shared_ptr<Connection>> connections_;
  static vector<shared_ptr<Connection>> recycle_connections_;
  static set<string> to_connect_set_;
  bool is_first_client_ = false;
  static int task_count_;
  static std::mutex task_mtx_;
  static std::condition_variable task_cv_;

  string cid_ = ""; // client id
  string sid_ = ""; // server id (connect to)
  string sslid_ = ""; // from sslid to sid

  SSL_CTX* ctx_ = nullptr;
  SSL* ssl_ = nullptr;

};

class SSLClient : public TCPClient {
  using TCPClient::TCPClient;

 public:
  virtual bool init_ssl();
  SSLClient(const std::string& task_id, const std::string& node_id, const std::string& ip, int port);
  ~SSLClient();
};

} // namespace io
} // namespace rosetta
