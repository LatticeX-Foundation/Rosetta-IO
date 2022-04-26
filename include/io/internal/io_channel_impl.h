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

#define USE_EMP_IO 0

#include "io/channel.h"
#include "io/internal/config.h"

#if USE_EMP_IO
#include "cc/third_party/emp-toolkit/emp-tool/emp-tool/emp-tool.h"
#endif

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
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
class BasicIO;

class TCPChannel : public IChannel{
  public:
#if USE_EMP_IO
    TCPChannel(string task_id, string ip, int port, shared_ptr<emp::NetIO> net_io, const string& node_id, shared_ptr<io::ChannelConfig> config)
      : _net_io(net_io), node_id_(node_id), config_(config), task_id_(task_id), ip_(ip), port_(port){ };
#else
    TCPChannel(string task_id, shared_ptr<io::BasicIO> net_io, const string& node_id, shared_ptr<io::ChannelConfig> config)
      : _net_io(net_io), node_id_(node_id), config_(config), task_id_(task_id){ };
#endif

    virtual ~TCPChannel();

    virtual void SetErrorCallback(error_callback error_cb) {}

    virtual int64_t Recv(const char* node_id, const char* id, char* data, uint64_t length, int64_t timeout = -1);

    virtual int64_t Send(const char* node_id, const char* id, const char* data, uint64_t length, int64_t timeout = -1);

    virtual void Flush();

    virtual const NodeIDVec* GetDataNodeIDs();

    virtual const NodeIDMap* GetComputationNodeIDs();

    virtual const NodeIDVec* GetResultNodeIDs();

    virtual const char* GetCurrentNodeID();
    
    virtual const NodeIDVec* GetConnectedNodeIDs();

    void SetConnectedNodeIDs(const vector<string>& connected_nodes) { connected_nodes_ = connected_nodes; }

  private:
    const vector<string>& getDataNodeIDs();

    const map<string, int>& getComputationNodeIDs();

    const vector<string>& getResultNodeIDs();

    const string& getCurrentNodeID();
    
    const vector<string>& getConnectedNodeIDs();

#if USE_EMP_IO
    shared_ptr<emp::NetIO> GetSubIO(string id);
#endif

  private:
#if USE_EMP_IO
    shared_ptr<emp::NetIO> _net_io = nullptr;
    std::mutex msgid2io_mutex_;
    map<string, shared_ptr<emp::NetIO>> msgid2io_;
    map<string, int> msgid2port_;
    string ip_;
    int port_;
    int port_offset = 100;
#else
    shared_ptr<io::BasicIO> _net_io = nullptr;
#endif
    shared_ptr<io::ChannelConfig> config_ = nullptr;
    vector<string> connected_nodes_;
    string node_id_;
    string task_id_;

};
} // namespace io
} // namespace rosetta
