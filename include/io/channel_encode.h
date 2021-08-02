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
#include <string>
#include <vector>
#include <map>
#include "io/channel.h"
using namespace std;
/**
 * @brief encode c++ standard string to C string
 * @param str c++ standard string
 * @return C string
*/
const char* encode_string(const string& str) {
  char* res = new char[str.size() + 1];
  memset(res, 0, str.size() + 1);
  memcpy(res, str.data(), str.size());
  return res;
}

/**
 * @brief encode c++ vector<string> to NodeIDVec
 * @param vec data to be encoded
 * @return a pointer to NodeIDVec
*/
const NodeIDVec* encode_vector(const vector<string>& vec) {
  NodeIDVec* res = new NodeIDVec();
  res->node_count = vec.size();
  res->node_ids = new char*[vec.size()];
  for (int i = 0; i < vec.size(); i++) {
    res->node_ids[i] = new char[vec[i].size() + 1];
    memset(res->node_ids[i], 0, vec[i].size() + 1);
    memcpy(res->node_ids[i], vec[i].data(), vec[i].size());
  }
  return res;
}

/**
 * @brief encode C++ map<string,int> to NodeIDMap
 * @param m data to be encoded
 * @return a pointer to NodeIDMap
*/
const NodeIDMap* encode_map(const map<string, int>& m) {
  NodeIDMap* pointer = new NodeIDMap();
  pointer->node_count = m.size();
  pointer->pairs = new NodeIDPair*[m.size()];
  int i = 0;
  for (auto iter = m.begin(); iter != m.end(); iter++) {
    pointer->pairs[i] = new NodeIDPair();
    pointer->pairs[i]->node_id = new char[iter->first.size() + 1];
    memset(pointer->pairs[i]->node_id, 0, iter->first.size() + 1);
    memcpy(pointer->pairs[i]->node_id, iter->first.data(), iter->first.size());
    pointer->pairs[i]->party_id = iter->second;
    i++;
  }
  return pointer;
}
