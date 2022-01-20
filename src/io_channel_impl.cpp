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
#include "io/internal/io_channel_impl.h"
#include "io/internal/net_io.h"
#include "io/internal/config.h"
#include "io/channel.h"
#include "io/internal_channel.h"
#include <set>
using namespace rosetta::io;
using namespace rosetta;

static int get_hex_index(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'Z') {
    return 10 + c - 'A';
  } else if (c >= 'a' && c <= 'z') {
    return 10 + c - 'a';
  }
  return 0;
}

static char get_char(char c1, char c2) {
  return get_hex_index(c1) << 4 | get_hex_index(c2);
}

static string get_binary_string(const string& str) {
  string ret;
  ret.resize(str.size() / 2);
  for (int i = 0; i < str.size(); i += 2) {
    ret[i / 2] = get_char(str[i], str[i + 1]);
  }
  return ret;
}


namespace rosetta {

TCPChannel::~TCPChannel() {
}

ssize_t TCPChannel::Recv(const char* node_id, const char* id, char* data, uint64_t length, int64_t timeout) {
#if USE_EMP_IO
  _net_io->recv_data(data, length);
  return length;
#else
  return _net_io->recv(node_id, data, length, get_binary_string(id), timeout); 
#endif
}

ssize_t TCPChannel::Send(const char* node_id, const char* id, const char* data, uint64_t length, int64_t timeout) {
#if USE_EMP_IO
  _net_io->send_data(data, length);
  //_net_io->flush();
  return length;
#else
  return _net_io->send(node_id, data, length, get_binary_string(id), timeout);
#endif
}

const vector<string>& TCPChannel::getDataNodeIDs() {
  return config_->data_nodes_;
}

const map<string, int>& TCPChannel::getComputationNodeIDs() {
  return config_->compute_nodes_;
}

const vector<string>& TCPChannel::getResultNodeIDs() {
  return config_->result_nodes_;
}

const string& TCPChannel::getCurrentNodeID() {
  return node_id_;
}

const vector<string>& TCPChannel::getConnectedNodeIDs() {
  return connected_nodes_;
}

} // namespace rosetta
