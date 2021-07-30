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
#include "io/channel.h"
#include "io/internal/config.h"
#include "io/internal_channel.h"
#include "io/internal/net_io.h"
#include "io/internal/io_channel_impl.h"
#include "string.h"
#include <mutex>
#include "io/internal/channel_interface.h"
#include "io/channel_encode.h"
using namespace std;
map<IChannel*, const char*> g_current_node_map;
map<IChannel*, const NodeIDVec*> g_data_node_map;
map<IChannel*, const NodeIDMap*> g_computation_node_map;
map<IChannel*, const NodeIDVec*> g_result_node_map;
map<IChannel*, const NodeIDVec*> g_connected_node_map;
std::mutex g_current_node_mutex;
std::mutex g_data_node_mutex;
std::mutex g_computation_node_mutex;
std::mutex g_result_node_mutex;
std::mutex g_connected_node_mutex;

static map<IChannel*, string> g_channel2task;
static set<string> g_creating_task;
static std::mutex g_channel2task_mtx;
static std::condition_variable g_channel2task_cv;
static IChannel* CreateChannel(const string& task_id, const rosetta::io::NodeInfo& nodeInfo, const vector<rosetta::io::NodeInfo>& clientInfos, const vector<rosetta::io::NodeInfo>& serverInfos, error_callback error_callback, shared_ptr<rosetta::io::ChannelConfig> config) {
  // check channel pool
  {
    std::unique_lock<std::mutex> lck(g_channel2task_mtx);
    g_channel2task_cv.wait(lck, [&](){
      auto iter = g_creating_task.find(task_id);
      if (iter == g_creating_task.end()) {
        return true;
      }
      return false;
    });
    for (auto iter = g_channel2task.begin(); iter != g_channel2task.end(); iter++) {
      if (iter->second == task_id) {
        log_warn << "the channel with task id " << task_id << " has already been created";
        return iter->first;
      }
    }
    g_creating_task.insert(task_id);
  }
  log_info << "begin create channel with task id " << task_id;
  shared_ptr<rosetta::io::BasicIO> net_io =  nullptr;

#if USE_SSL_SOCKET // (USE_GMTASSL || USE_OPENSSL)
  if (netutil::is_enable_ssl_socket()) {
    net_io = make_shared<rosetta::io::SSLParallelNetIO>(task_id, nodeInfo, clientInfos, serverInfos, error_callback);
  } else {
    net_io = make_shared<rosetta::io::ParallelNetIO>(task_id, nodeInfo, clientInfos, serverInfos, error_callback);
  }
#else
  net_io = make_shared<rosetta::io::ParallelNetIO>(task_id, nodeInfo, clientInfos, serverInfos, error_callback);
#endif

  if (net_io->init()) {
    rosetta::io::TCPChannel* tcp_channel = new rosetta::io::TCPChannel(net_io, nodeInfo.id, config);
    vector<string> connected_nodes(clientInfos.size() + serverInfos.size());
    for (int i = 0; i < clientInfos.size(); i++) {
      connected_nodes[i] = clientInfos[i].id;
    }

    int connected_offset = clientInfos.size();
    for (int i = 0; i < serverInfos.size(); i++) {
      connected_nodes[connected_offset + i] = serverInfos[i].id;
    }
    tcp_channel->SetConnectedNodeIDs(connected_nodes);
    // put channel into pool
    {
      std::unique_lock<std::mutex> lck(g_channel2task_mtx);
      g_channel2task.insert(std::pair<IChannel*, string>(tcp_channel, task_id));
      auto iter = g_creating_task.find(task_id);
      g_creating_task.erase(iter);
      g_channel2task_cv.notify_all();
    }
    log_info << "end create channel with task id " << task_id;
    return tcp_channel;
  }
  
  log_error << "create io channel with task id " << task_id << " failed, init failed!!!";
  return nullptr;
}

void CopyNodeInfo(rosetta::io::NodeInfo& dst, const rosetta::io::Node& src) {
  dst.id = src.NODE_ID;
  dst.address = src.HOST;
  dst.port = src.PORT;
}
static void DestroyCurrentNode(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_current_node_mutex);
  auto iter = g_current_node_map.find(channel);
  if (iter != g_current_node_map.end()) {
    delete []iter->second;
    g_current_node_map.erase(iter);
  }
}

static void DestroyDataNodes(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_data_node_mutex);
  auto iter = g_data_node_map.find(channel);
  if (iter != g_data_node_map.end()) {
    for (int i = 0; i < iter->second->node_count; i++) {
      delete []iter->second->node_ids[i];
    }
    delete []iter->second->node_ids;
    delete iter->second;
    g_data_node_map.erase(iter);
  }
}

static void DestroyComputationNodes(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_computation_node_mutex);
  auto iter = g_computation_node_map.find(channel);
  if (iter != g_computation_node_map.end()) {
    for (int i = 0; i < iter->second->node_count; i++) {
      delete []iter->second->pairs[i]->node_id;
      delete iter->second->pairs[i];
    }
    delete []iter->second->pairs;
    delete iter->second;
    g_computation_node_map.erase(iter);
  }
}

static void DestroyResultNodes(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_result_node_mutex);
  auto iter = g_result_node_map.find(channel);
  if (iter != g_result_node_map.end()) {
    for (int i = 0; i < iter->second->node_count; i++) {
      delete []iter->second->node_ids[i];
    }
    delete []iter->second->node_ids;
    delete iter->second;
    g_result_node_map.erase(iter);
  }
}

static void DestroyConnectedNodes(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_connected_node_mutex);
  auto iter = g_connected_node_map.find(channel);
  if (iter != g_connected_node_map.end()) {
    for (int i = 0; i < iter->second->node_count; i++) {
      delete []iter->second->node_ids[i];
    }
    delete []iter->second->node_ids;
    delete iter->second;
    g_connected_node_map.erase(iter);
  }
}

void DestroyInternalChannel(IChannel* channel) {
  // remove channel from pool
  string task_id = "";
  {
    std::unique_lock<std::mutex> lck(g_channel2task_mtx);
    auto iter = g_channel2task.find(channel);
    if (iter == g_channel2task.end()) {
      log_error << "can not find channel";
    }
    task_id = iter->second;
    g_channel2task.erase(iter);
  }
  log_info << "begin destroy channel with task id " << task_id;
  DestroyCurrentNode(channel);
  DestroyDataNodes(channel);
  DestroyComputationNodes(channel);
  DestroyResultNodes(channel);
  DestroyConnectedNodes(channel);
  delete channel;
  log_info << "end destroy channel with task id " << task_id;
}

IChannel* CreateInternalChannel(const char* task_id, const char* node_id, const char* config_str, error_callback error_cb) {
  shared_ptr<rosetta::io::ChannelConfig> config = make_shared<rosetta::io::ChannelConfig>(node_id, config_str);
  rosetta::io::NodeInfo node_info;
  vector<rosetta::io::NodeInfo> clientInfos;
  vector<rosetta::io::NodeInfo> serverInfos;
  const rosetta::io::Node& node = config->GetNode(node_id);
  rosetta::io::NODE_TYPE node_type = config->GetPrimaryNodeType(node_id);

  CopyNodeInfo(node_info, node);
  rosetta::io::NodeInfo tmp;
  if ( node_type == rosetta::io::NODE_TYPE_DATA) {
    for (int i = 0; i < config->compute_config_.P.size(); i++) {
      string nid = config->compute_config_.P[i].NODE_ID;
      CopyNodeInfo(tmp, config->compute_config_.P[i]);
      if (node.PORT <= 0 || (tmp.port > 0 && node_id < nid)) {
        serverInfos.push_back(tmp);
      } else {
        clientInfos.push_back(tmp);
      }
    }
  } else if (node_type == rosetta::io::NODE_TYPE_COMPUTE) {
    for (int i = 0; i < config->data_config_.P.size(); i++) {
      string nid = config->data_config_.P[i].NODE_ID;
      CopyNodeInfo(tmp, config->data_config_.P[i]);
      if (node.PORT <= 0 || (tmp.port > 0 && node_id < nid)) {
        serverInfos.push_back(tmp);
      } else {
        clientInfos.push_back(tmp);
      }
    }

    for (int i = 0; i < config->compute_config_.P.size(); i++) {
      string nid = config->compute_config_.P[i].NODE_ID;
      if (node_id != nid) {
        CopyNodeInfo(tmp, config->compute_config_.P[i]);
        if (node.PORT <= 0 || (tmp.port > 0 && node_id < nid)) {
          serverInfos.push_back(tmp);
        } else {
          clientInfos.push_back(tmp);
        }
      }
    }

    for (int i = 0; i < config->result_config_.P.size(); i++) {
      string nid = config->result_config_.P[i].NODE_ID;
      CopyNodeInfo(tmp, config->result_config_.P[i]);
      if (node.PORT <= 0 || (tmp.port > 0 && node_id < nid)) {
        serverInfos.push_back(tmp);
      } else {
        clientInfos.push_back(tmp);
      }
    }
  } else if (node_type == rosetta::io::NODE_TYPE_RESULT) {
    for (int i = 0; i < config->compute_config_.P.size(); i++) {
      string nid = config->compute_config_.P[i].NODE_ID;
      CopyNodeInfo(tmp, config->compute_config_.P[i]);
      if (node.PORT <= 0 || (tmp.port > 0 && node_id < nid)) {
        serverInfos.push_back(tmp);
      } else {
        clientInfos.push_back(tmp);
      }
    }
  } else if (node_type == rosetta::io::NODE_TYPE_INVALID) {
    throw other_exp("can not find node in config file!");
  } 
  return CreateChannel(task_id, node_info, clientInfos, serverInfos, error_cb, config);
}

 /**
 * @brief Set error callback for error handle.
 * @param channel Channel Handler
 * @param error_cb error callback to handle error.
 * @note Should set callback from python to c++, Rosetta internal should not set error callback.
*/
void SetErrorCallback(IChannel* channel, error_callback error_cb) {
  channel->SetErrorCallback(error_cb);
}

/**
 * @brief Recv receive a message from message queue， for the target node (blocking for timeout microseconds, default waiting forever)
 * @param channel Channel Handler
 * @param node_id target node id for message receiving.
 * @param id identity of a message, could be a task id or message id.
 * @param data buffer to receive a message.
 * @param length data length expect to receive
 * @param timeout timeout to receive a message.
 * @return 
 *  return message length if receive a message successfully
 *  0 if peer is disconnected  
 *  -1 if it gets a exception or error
*/
int64_t Recv(IChannel* channel, const char* node_id, const char* id, char* data, uint64_t length, int64_t timeout) {
  return channel->Recv(node_id, id, data, length, timeout);
}

/**
 * @brief Send send a message to target node
 * @param channel Channel Handler
 * @param node_id target node id for message receiving
 * @param id identity of a message, could be a task id or message id.
 * @param data buffer to send
 * @param length data length expect to send
 * @param timeout timeout to receive a message.
 * @return 
 *  return length of data has been sent if send a message successfully
 *  -1 if gets exceptions or error
*/
int64_t Send(IChannel* channel, const char* node_id, const char* id, const char* data, uint64_t length, int64_t timeout) {
  return channel->Send(node_id, id, data, length, timeout);
}

/**
 * @brief get node id of all the data nodes
 * @param channel Channel Handler
 * @return
 * return node id of all the data nodes
*/
const NodeIDVec* GetDataNodeIDs(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_data_node_mutex);
  auto iter = g_data_node_map.find(channel);
  if ( iter == g_data_node_map.end()) {
    const NodeIDVec* nodes = encode_vector(channel->GetDataNodeIDs());
    g_data_node_map.insert(std::pair<IChannel*, const NodeIDVec*>(channel, nodes));
    return nodes;
  }
  return iter->second;
}

/**
 * @brief get node id and party id of all the computation nodes
 * @param channel Channel Handler
 * @return
 * return node id and party id of all the computation nodes
 * string  indicates node id and int indicates party id
*/
const NodeIDMap* GetComputationNodeIDs(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_computation_node_mutex);
  auto iter = g_computation_node_map.find(channel);
  if ( iter == g_computation_node_map.end()) {
    const NodeIDMap* nodes = encode_map(channel->GetComputationNodeIDs());
    g_computation_node_map.insert(std::pair<IChannel*, const NodeIDMap*>(channel, nodes));
    return nodes;
  }
  return iter->second;
}

/**
 * @brief get node id of all the result nodes
 * @param channel Channel Handler
 * @return
 * return node id of all the result nodes
*/
const NodeIDVec* GetResultNodeIDs(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_result_node_mutex);
  auto iter = g_result_node_map.find(channel);
  if ( iter == g_result_node_map.end()) {
    const NodeIDVec* nodes = encode_vector(channel->GetResultNodeIDs());
    g_result_node_map.insert(std::pair<IChannel*, const NodeIDVec*>(channel, nodes));
    return nodes;
  }
  return iter->second;
}
/**
 * @brief get node id of the current node
 * @param channel Channel Handler
 * @return
 * return node id of the current node
*/
const char* GetCurrentNodeID(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_current_node_mutex);
  auto iter = g_current_node_map.find(channel);
  if ( iter == g_current_node_map.end()) {
    const char* nodes = encode_string(channel->GetCurrentNodeID());
    g_current_node_map.insert(std::pair<IChannel*, const char*>(channel, nodes));
    return nodes;
  }
  return iter->second;
}

/**
 * @brief get node id of all the nodes establishing connection with the current node
 * @param channel Channel Handler
 * @return
 * return node id of all the nodes establishing connection with the current node
*/
const NodeIDVec* GetConnectedNodeIDs(IChannel* channel) {
  std::unique_lock<std::mutex> lck(g_connected_node_mutex);
  auto iter = g_connected_node_map.find(channel);
  if ( iter == g_connected_node_map.end()) {
    const NodeIDVec* nodes = encode_vector(channel->GetConnectedNodeIDs());
    g_connected_node_map.insert(std::pair<IChannel*, const NodeIDVec*>(channel, nodes));
    return nodes;
  }
  return iter->second;
}

