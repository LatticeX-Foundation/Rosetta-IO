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
#include "io/internal/logger.h"
#include "io/internal/rtt_exceptions.h"
#include "io/internal/config.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
using namespace rapidjson;

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include <unistd.h>

namespace rosetta {
namespace io {

namespace {
bool is_file_exist(const string& filepath) {
  if (filepath.empty())
    return false;
  return (access(filepath.c_str(), F_OK) == 0);
}
void if_key_not_exist_then_exit(bool must_exist, const char* key) {
  if (must_exist) {
    log_error << "key[" << key << "] not exist!\n";
    throw other_exp("key[" + string(key) + "] not exist!");
  }
}
std::string GetString(
  rapidjson::Value& v,
  const char* key,
  const char* default_value = "",
  bool must_exist = false) {
  if (v.HasMember(key)) {
    return v[key].GetString();
  }
  if_key_not_exist_then_exit(must_exist, key);
  return std::string(default_value);
}
int GetInt(rapidjson::Value& v, const char* key, int default_value = 0, bool must_exist = false) {
  if (v.HasMember(key)) {
    return v[key].GetInt();
  }
  if_key_not_exist_then_exit(must_exist, key);
  return default_value;
}
float GetFloat(
  rapidjson::Value& v,
  const char* key,
  float default_value = 0.0f,
  bool must_exist = false) {
  if (v.HasMember(key)) {
    return v[key].GetFloat();
  }
  if_key_not_exist_then_exit(must_exist, key);
  return default_value;
}
bool GetBool(
  rapidjson::Value& v,
  const char* key,
  bool default_value = 0,
  bool must_exist = false) {
  if (v.HasMember(key)) {
    return v[key].GetBool();
  }
  if_key_not_exist_then_exit(must_exist, key);
  return default_value;
}
} // namespace


std::string ComputeNodeConfig::to_string() {
  std::stringstream sss;

  for (int i = 0; i < P.size(); i++) {
    sss << "\n        P" << i << " NAME: " << P[i].NAME;
    sss << "\n        P" << i << " HOST: " << P[i].HOST;
    sss << "\n        P" << i << " PORT: " << P[i].PORT;
  }
  sss << "\n";
  return sss.str();
}

std::string ResultNodeConfig::to_string() {
  std::stringstream sss;

  for (int i = 0; i < P.size(); i++) {
    sss << "\n        P" << i << " NAME: " << P[i].NAME;
    sss << "\n        P" << i << " HOST: " << P[i].HOST;
    sss << "\n        P" << i << " PORT: " << P[i].PORT;
  }
  sss << "\n";
  return sss.str();
}

std::string DataNodeConfig::to_string() {
  std::stringstream sss;

  for (int i = 0; i < P.size(); i++) {
    sss << "\n        P" << i << " NAME: " << P[i].NAME;
    sss << "\n        P" << i << " HOST: " << P[i].HOST;
    sss << "\n        P" << i << " PORT: " << P[i].PORT;
  }
  sss << "\n";
  return sss.str();
}

ChannelConfig::ChannelConfig(const string& node_id, const string& config_json) {
  //! @attention use node_id__ID = node_id
  bool ret = load(node_id, config_json);
  if (!ret) {
    throw other_exp("ChannelConfig load2 config json failed!");
  }
}

NODE_TYPE ChannelConfig::GetPrimaryNodeType(const string& node_id) {
  for (int i = 0; i < pure_data_nodes_.size(); i++) {
    if (node_id == pure_data_nodes_[i]) {
      return NODE_TYPE_DATA;
    }
  }

  for (auto iter = compute_nodes_.begin(); iter != compute_nodes_.end(); iter++) {
    if (node_id == iter->first) {
      return NODE_TYPE_COMPUTE;
    }
  }

  for (int i = 0; i < pure_result_nodes_.size(); i++) {
    if (node_id == pure_result_nodes_[i]) {
      return NODE_TYPE_RESULT;
    }
  }

  return NODE_TYPE_INVALID;
}

const Node& ChannelConfig::GetNode(const string& node_id) {
  for (int i = 0; i < data_config_.P.size(); i++) {
    if (node_id == data_config_.P[i].NODE_ID)
      return data_config_.P[i];
    log_debug << "data node id:" << data_config_.P[i].NODE_ID ;
  }

  for (int i = 0; i < compute_config_.P.size(); i++) {
    if (node_id == compute_config_.P[i].NODE_ID)
      return compute_config_.P[i];
    log_debug << "compute node id:" << compute_config_.P[i].NODE_ID ;
  }

  for (int i = 0; i < result_config_.P.size(); i++) {
    if (node_id == result_config_.P[i].NODE_ID)
      return result_config_.P[i];
    log_debug << "result node id:" << result_config_.P[i].NODE_ID ;
  }
  log_debug << "node_id: " << node_id ;
  throw other_exp("can not find node in config!");
}

bool ChannelConfig::load(const string& node_id, const string& config_file) {
  string sjson(config_file);
  ifstream ifile(config_file);
  if (!ifile.is_open()) {
    //log_warn << "open " << config_file << " error!\n";
    log_debug << "try to load as json string" ;
  } else {
    sjson = "";
    while (!ifile.eof()) {
      string s;
      getline(ifile, s);
      sjson += s;
    }
    ifile.close();
  }

  Document doc;
  if (doc.Parse(sjson.data()).HasParseError()) {
    log_error << "parser " << config_file << " error!\n";
    return false;
  }

  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  string data = buffer.GetString();

  if (!parse(doc)) {
    log_error << "parse error" ;
    return false;
  }

  return true;
}

bool ChannelConfig::parse_node_info(Document& doc) {
  if (doc.HasMember("NODE_INFO") && doc["NODE_INFO"].IsArray()) {
    Value& Nodes = doc["NODE_INFO"];

    // nodes
    for (int i = 0; i < Nodes.Size(); i++) {
      NodeInfoConfig cfg;
      Value& Node = Nodes[i];
      cfg.node_.NODE_ID = GetString(Node, "NODE_ID", "", false);
      log_debug << "node info parse:" << cfg.node_.NODE_ID ;
      cfg.node_.NAME = GetString(Node, "NAME", (std::string("Node-") + std::to_string(i)).c_str(), false);
      cfg.node_.HOST = GetString(Node, "HOST", "127.0.0.1", false);
      cfg.node_.PORT = GetInt(Node, "PORT", 0, false);
      node_info_config_.insert(std::pair<string, NodeInfoConfig>(cfg.node_.NODE_ID, cfg));
    }
    log_debug << "parse " << Nodes.Size() << " node info success" ;

  }
  return true;
}

bool ChannelConfig::parse_data(Document& doc) {
  if (doc.HasMember("DATA_NODES") && doc["DATA_NODES"].IsArray()) {
    Value& Nodes = doc["DATA_NODES"];

    // nodes
    data_nodes_.resize(Nodes.Size());
    data_config_.P.reserve(Nodes.Size());
    for (int i = 0; i < Nodes.Size(); i++) {
      Value& Node = Nodes[i];
      data_nodes_[i] = Node.GetString();
      if (node_info_config_.find(data_nodes_[i]) != node_info_config_.end()) {
        data_config_.P.push_back(node_info_config_[data_nodes_[i]].node_);
      } else {
        log_error << "can not find node info, node id:" << data_nodes_[i] ;
      }
    }
    log_debug << "parse " << Nodes.Size() << " data success" ;

  }
  return true;
}

bool ChannelConfig::parse_compute(Document& doc) {
  if (doc.HasMember("COMPUTATION_NODES") && doc["COMPUTATION_NODES"].IsObject()) {
    Value& Nodes = doc["COMPUTATION_NODES"];

    // nodes
    for (auto iter = Nodes.MemberBegin(); iter != Nodes.MemberEnd(); iter++) {
      string name = iter->name.GetString();
      int value = iter->value.GetInt();
      compute_nodes_.insert(std::pair<string, int>(name, value));
    }

    compute_config_.P.reserve(compute_nodes_.size());
    int i = 0;
    for (auto iter = compute_nodes_.begin(); iter != compute_nodes_.end(); iter++, i++) {
      if (node_info_config_.find(iter->first) != node_info_config_.end()) {
        compute_config_.P.push_back(node_info_config_[iter->first].node_);
      } else {
        log_error << "can not find node info, node id:" << iter->first ;
      }
    }
    log_debug << "parse " << " computation success" ;
  }
  return true;
}

bool ChannelConfig::parse_result(Document& doc) {
  if (doc.HasMember("RESULT_NODES") && doc["RESULT_NODES"].IsArray()) {
    Value& Nodes = doc["RESULT_NODES"];

    // nodes
    result_nodes_.resize(Nodes.Size());
    result_config_.P.reserve(Nodes.Size());
    for (int i = 0; i < Nodes.Size(); i++) {
      Value& Node = Nodes[i];
      result_nodes_[i] = Node.GetString();
      if (node_info_config_.find(result_nodes_[i]) != node_info_config_.end()) {
        result_config_.P.push_back(node_info_config_[result_nodes_[i]].node_);
      } else {
        log_error << "can not find node info, node id:" << result_nodes_[i] ;
      }
    }
    log_debug << "parse " << Nodes.Size() << " result success" ;
  }
  return true;
}

bool ChannelConfig::parse_connect_params(Document& doc) {
  if (doc.HasMember("CONNECT_PARAMS") && doc["CONNECT_PARAMS"].IsObject()) {
    Value& connect_param = doc["CONNECT_PARAMS"];
    if (connect_param.HasMember("TIMEOUT") && connect_param["TIMEOUT"].IsInt()) {
      int timeout = connect_param["TIMEOUT"].GetInt();
      if (timeout > 0) {
        connect_timeout_ = timeout * 1000;
      }
    }

    if (connect_param.HasMember("RETRIES") && connect_param["RETRIES"].IsInt()) {
      int retries = connect_param["RETRIES"].GetInt();
      if (retries > 0) {
        connect_retries_ = retries;
      }
    }
  }
  log_debug << "connect timeout:" << connect_timeout_ << "ms, connect retries:" << connect_retries_;

  return true;
}

void ChannelConfig::process_node_type() {
  pure_data_nodes_ = data_nodes_;
  pure_result_nodes_ = result_nodes_;

  for (auto iter = pure_data_nodes_.begin(); iter != pure_data_nodes_.end(); ) {
    if (compute_nodes_.find(*iter) != compute_nodes_.end()) {
      iter = pure_data_nodes_.erase(iter);
    } else {
      iter++;
    }
  }

  for (auto iter = pure_result_nodes_.begin(); iter != pure_result_nodes_.end();) {
    if (compute_nodes_.find(*iter) != compute_nodes_.end() 
      || std::find(pure_data_nodes_.begin(), pure_data_nodes_.end(), *iter) != pure_data_nodes_.end()) {
      iter = pure_result_nodes_.erase(iter);
    } else {
      iter++;
    }
  }

  for (auto iter = data_config_.P.begin(); iter != data_config_.P.end();) {
    if (std::find(compute_config_.P.begin(), compute_config_.P.end(), *iter) != compute_config_.P.end()) {
      iter = data_config_.P.erase(iter);
    } else {
      iter++;
    }
  }

  for (auto iter = result_config_.P.begin(); iter != result_config_.P.end();) {
    if (std::find(compute_config_.P.begin(), compute_config_.P.end(), *iter) != compute_config_.P.end()
      || std::find(data_config_.P.begin(), data_config_.P.end(), *iter) != data_config_.P.end()) {
      iter = result_config_.P.erase(iter);
    } else {
      iter++;
    }
  }
   
  log_debug << "pure data node size:" << pure_data_nodes_.size();
  for (int i = 0; i < pure_data_nodes_.size(); i++) {
    log_debug << "pure data node:" << pure_data_nodes_[i];
  }
  log_debug << "computation node size:" << compute_nodes_.size();
  for (auto iter = compute_nodes_.begin(); iter != compute_nodes_.end(); iter++) {
    log_debug << "computation node:" << iter->first;
  }
  log_debug << "pure result node size:" << pure_result_nodes_.size();
  for (int i = 0; i < pure_result_nodes_.size(); i++) {
    log_debug << "result node:" << pure_result_nodes_[i];
  }
}

bool ChannelConfig::parse(Document& doc) {
  if (!parse_node_info(doc)) {
    log_error << "parse node info error" ;
  }

  if (!parse_data(doc)) {
    log_error << "parse data error" ;
    return false;
  }

  if (!parse_compute(doc)) {
    log_error << "parse compute error" ;
    return false;
  }

  if (!parse_result(doc)) {
    log_error << "parse result error" ;
    return false;
  }

  if (!parse_connect_params(doc)) {
    log_error << "parse connect params error";
    return false;
  }

  process_node_type();

  // fmt_print();
  return true;
}

void ChannelConfig::fmt_print() {
  log_debug << "=======================================" ;
  log_debug << data_config_.to_string();
  log_debug << compute_config_.to_string();
  log_debug << result_config_.to_string();
  log_debug << "=======================================" ;
}

}
} // namespace rosetta
