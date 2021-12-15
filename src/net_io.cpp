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
#include "io/internal/net_io.h"
#include <set>

namespace rosetta {
namespace io {

void BasicIO::close() {
  vector<thread> client_threads(clients.size());
  auto client_f = [&](shared_ptr<TCPClient> client) -> void {
    client->close();
  };
  int i = 0;
  for (auto iter = clients.begin(); iter != clients.end(); iter++, i++) {
    client_threads[i] = thread(client_f, iter->second);
  }

  for (int i = 0; i < client_threads.size(); i++) {
    client_threads[i].join();
  }
  client_threads.clear();
  clients.clear();

  if (server != nullptr)
    server->stop();
}

BasicIO::~BasicIO() {
  close();
}

BasicIO::BasicIO(const string& task_id, const NodeInfo &node_id, const vector<NodeInfo>& client_infos, const vector<NodeInfo>& server_infos, error_callback error_callback, const shared_ptr<ChannelConfig>& channel_config)
  : task_id_(task_id), node_info_(node_id), client_infos_(client_infos), server_infos_(server_infos), handler(error_callback), channel_config_(channel_config) {}

bool BasicIO::init() {

  init_inner();

  vector<string> expected_cids;
  for (int i = 0; i < client_infos_.size(); i++)
  {
    log_debug << node_info_.id << " expected cids:" << client_infos_[i].id ;
    expected_cids.push_back(client_infos_[i].id);
  }
  std::set<string> cid_set(expected_cids.begin(), expected_cids.end());

  vector<string> expected_sids;
  for (int i = 0; i < server_infos_.size(); i++) {
    log_debug << node_info_.id << " expected sids:" << server_infos_[i].id ;
    expected_sids.push_back(server_infos_[i].id);
  }
  std::set<string> sid_set(expected_sids.begin(), expected_sids.end());
  std::vector<string> expected_ids(expected_sids.begin(), expected_sids.end());
  expected_ids.insert(expected_ids.end(), expected_cids.begin(), expected_cids.end());

  // init server with each node_id's port
  if (is_ssl_io_)
    server = make_shared<SSLServer>();
  else
    server = make_shared<TCPServer>();

  //server->set_server_cert(server_cert_);
  //server->set_server_prikey(server_prikey_, server_prikey_password_);
  server->set_expected_cids(expected_cids);
  server->setsid(node_info_.id);
  log_debug << "begin to start" ;

  if (!server->start(task_id_, node_info_.port, handler))
    return false;

  // init clients, each have parties's clients. one which i==node_id is not use
  /////////////////////////////////////////////////////
  vector<thread> client_threads(server_infos_.size() + client_infos_.size());

  bool init_client_ok = true;
  auto conn_f = [&](const string &i, int j) -> bool {
    if (sid_set.find(i) != sid_set.end()) {
      log_debug << node_info_.id <<" start to connect to " << i ;
      shared_ptr<TCPClient> client = nullptr;
      if (is_ssl_io_)
        client = make_shared<SSLClient>(task_id_, i, server_infos_[j].address, server_infos_[j].port);
      else
        client = make_shared<TCPClient>(task_id_, i, server_infos_[j].address, server_infos_[j].port);

      client->setcid(node_info_.id);
      client->setsid(i);
      client->setsslid(node_info_.id);
      {
        unique_lock<mutex> lck(clients_mtx_);
        clients.insert(std::pair<string, shared_ptr<TCPClient>>(i, client));
      }
      if (!client->connect(channel_config_->connect_timeout_, channel_config_->connect_retries_)) {
        init_client_ok = false;
        return false;
      }

      // and connection to connection map
      {
        shared_ptr<Connection> conn = clients[i]->get_connection() ;
        connection_map[i] = conn;
        if (clients[i]->is_first_connect()) {
          server->add_connection_to_epoll(connection_map[i]);
        }
        if (conn == nullptr) {
          log_error << "client get null connection " << i ;
        }
      }
    }
    else if (cid_set.find(i) != cid_set.end()){
      while (server->put_connection(i) == false) {
        usleep(2000);
      }
      log_debug << node_info_.id << " waited to be connected by " << i ;
      
      // and connection to connection map
      {
        shared_ptr<Connection> conn = server->get_connection(i);
        if (conn == nullptr) {
          log_error << "server get null connection " << i ;
        }
        connection_map[i] = conn;
      }
    }
    return true;
  };
   
  for (int i = 0; i < expected_ids.size(); i++) {
     connection_map.insert(std::pair<string, shared_ptr<Connection>>(expected_ids[i], nullptr));
   }
  for (int i = 0; i < expected_ids.size(); i++) {
    client_threads[i] = thread(conn_f, expected_ids[i], i);
  }

  for (int i = 0; i < expected_ids.size(); i++) {
    client_threads[i].join();
  }

  if (!init_client_ok)
    return false;
  
  return true;
}

ssize_t BasicIO::recv(const string& node_id, char* data, uint64_t length, const string& id, int64_t timeout) {
  if (server->stoped())
    throw socket_exp("m server->stoped()");
  ssize_t ret = connection_map[node_id]->recv(id, data, length, timeout);
  return ret;
}

ssize_t BasicIO::send(const string& node_id, const char* data, uint64_t length, const string& id, int64_t timeout) {
  ssize_t ret = connection_map[node_id]->send(id, data, length, timeout);
  return ret;
}


} // namespace io
} // namespace rosetta
