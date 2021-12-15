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

/**
 * @todo
 * This file and its implement will be migrate to the `protocol/` directory.
 */
#include <string>
#include <vector>
#include <map>
using namespace std;

#include <rapidjson/document.h>
using rapidjson::Document;

namespace rosetta {
namespace io {
struct Node {
  string DESC = "";
  string NODE_ID = "";
  string NAME = "";
  string HOST = "";
  int PORT = 0;
  public:
    void copy_from(const Node& node) {
      DESC.assign(node.DESC);
      NODE_ID.assign(node.NODE_ID);
      NAME.assign(node.NAME);
      HOST.assign(node.HOST);
      PORT = node.PORT;
    }
    bool operator==(const Node& n) const {
      return NODE_ID == n.NODE_ID;
    }
};

class DataNodeConfig {
  public:
    vector<Node> P;
    std::string to_string();
};

class ResultNodeConfig {
  public:
    vector<Node> P;
    std::string to_string();
};
class NodeInfoConfig {
  public:
    Node node_;
    std::string to_string();
};

class ComputeNodeConfig {
 public:
  vector<Node> P;
  std::string to_string();

  // server and client certifications
  string SERVER_CERT = "certs/server-nopass.cert";
  string SERVER_PRIKEY = "certs/server-prikey";
  string SERVER_PRIKEY_PASSWORD = "123456";
};

struct NodeInfo {
  string id;
  string address;
  int port;

  NodeInfo() = default;
  NodeInfo(const string& node_id, const string& addr, int _port) 
    : id(node_id), address(addr), port(_port){}
};

enum NODE_TYPE {
  NODE_TYPE_INVALID,
  NODE_TYPE_DATA,
  NODE_TYPE_COMPUTE,
  NODE_TYPE_RESULT
};

class ChannelConfig {
 public:
  ChannelConfig(const string& node_id, const string& config_json);
  NODE_TYPE GetPrimaryNodeType(const string& node_id);
  const Node& GetNode(const string& node_id);

 private:
  bool load(const string& node_id, const string& config_file);
  bool parse(Document& doc);
  bool parse_node_info(Document& doc);
  bool parse_data(Document& doc);
  bool parse_compute(Document& doc);
  bool parse_result(Document& doc);
  bool parse_connect_params(Document& doc);
  void process_node_type();

 public:
  void fmt_print();

 public:
  map<string, NodeInfoConfig> node_info_config_;
  vector<string> data_nodes_;
  vector<string> pure_data_nodes_;
  map<string, int> compute_nodes_;
  vector<string> result_nodes_;
  vector<string> pure_result_nodes_;
  DataNodeConfig data_config_;
  ComputeNodeConfig compute_config_;
  ResultNodeConfig result_config_;
  int connect_timeout_ = 10 * 1000; // ms
  int connect_retries_ = 5;
};

}
} // namespace rosetta
