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

#include "io/internal/simple_buffer.h"
#include "io/internal/server.h"
#include "io/internal/client.h"
#include "io/internal/connection.h"
#include "io/internal/config.h"

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>
using namespace std;

/**
 * Users only need to include this one header file.
 * 
 * Provides NetIOInterface/SSLNetIO/ParallelNetIO/SSLParallelNetIO
 * 
 * Note:
 * 
 * Only the receiving of the server and the sending of the client are totally supported. 
 * 
 * The receiving of the client and the sending of the server are not totally supported.
 */
namespace rosetta {
namespace io {
/**
 * This is the basic class of Network IO.
 */
class BasicIO {
 public:
  virtual ~BasicIO();
  BasicIO() = default;

  BasicIO(const string& task_id, const NodeInfo &node_id, const vector<NodeInfo>& client_infos, const vector<NodeInfo>& server_infos, error_callback error_callback, const shared_ptr<ChannelConfig>& channel_config);

 protected:
  virtual bool init_inner() { return true; }

 public:
  /**
   * init the server and clients.
   */
  bool init();
  /**
   * close the connections.
   */
  void close();

  /**
   * set server certification
   * 
   * \param server_cert the file path of server certification
   */
  void set_server_cert(string server_cert) { server_cert_ = server_cert; }
  /**
   * set server private key
   * 
   * \param server_cert the file path of server private key \n
   * \param password optional. the password for server private key \n
   */
  void set_server_prikey(string server_prikey, string password = "") {
    server_prikey_ = server_prikey;
    server_prikey_password_ = password;
  }

 public:
  ssize_t recv(const string& node_id, char* data, uint64_t length, const string& id, int64_t timeout);
  ssize_t send(const string& node_id, const char* data, uint64_t length, const string& id, int64_t timeout);

 protected:
  int parties_ = -1;
  int port_ = -1;
  string task_id_ = "";
  bool is_parallel_io_ = false;
  bool is_ssl_io_ = false;
  NodeInfo node_info_;
  vector<NodeInfo> client_infos_;
  vector<NodeInfo> server_infos_;

  string server_cert_;
  string server_prikey_;
  string server_prikey_password_;

 protected:
  map<int, map<int, int>> node_id_cids_; // node_id id --> client ids <tid --> cid>
  shared_ptr<TCPServer> server = nullptr;
  map<string, shared_ptr<TCPClient>> clients;
  map<string, shared_ptr<Connection>> connection_map;
  error_callback handler = nullptr;
  std::mutex clients_mtx_;
  shared_ptr<ChannelConfig> channel_config_ = nullptr;
};

/**
 * General Net IO.
 */
class NetIOInterface : public BasicIO {
 public:
  using BasicIO::BasicIO;
  virtual ~NetIOInterface() = default;
};

/**
 * General Net IO with SSL.
 */
class SSLNetIO : public BasicIO {
 public:
  using BasicIO::BasicIO;
  virtual ~SSLNetIO() = default;

 protected:
  virtual bool init_inner() {
    is_ssl_io_ = true;
    return true;
  }
};

/**
 * Parallel Net IO.
 */
class ParallelNetIO : public BasicIO {
 public:
  using BasicIO::BasicIO;
  virtual ~ParallelNetIO() = default;

 protected:
  virtual bool init_inner() {
    is_parallel_io_ = true;
    return true;
  }
};

/**
 * Parallel Net IO with SSL.
 */
class SSLParallelNetIO : public BasicIO {
 public:
  using BasicIO::BasicIO;
  virtual ~SSLParallelNetIO() = default;

 protected:
  virtual bool init_inner() {
    is_ssl_io_ = true;
    is_parallel_io_ = true;
    return true;
  }
};

} // namespace io

} // namespace rosetta
