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
 * @brief decode C string to C++ standard string
 * @param pointer a pointer to C string
 * @return C++ standard string
*/
string decode_string(const char* pointer) {
  string res(pointer, strlen(pointer));
  return res;
}

/**
 * @brief decode NodeIDVec to C++ vector<string>
 * @param pointer a pointer to NodeIDVec
 * @return C++ vector<string> type
*/
vector<string> decode_vector(const NodeIDVec* pointer) {
  vector<string> res;
  res.resize(pointer->node_count);
  for (int i = 0; i < pointer->node_count; i++) {
    res[i] = string(pointer->node_ids[i], strlen(pointer->node_ids[i]));
  }
  return res;
}

/**
 * @brief decode NodeIDMap to C++ map<string, int> type
 * @param pointer a pointer to NodeIDMap
 * @return C++ map<string, int> type
*/
map<string, int> decode_map(const NodeIDMap* pointer) {
  map<string, int> res;
  for (int i = 0; i < pointer->node_count; i++) {
    string node_id(pointer->pairs[i]->node_id, strlen(pointer->pairs[i]->node_id));
    int party_id = pointer->pairs[i]->party_id;
    res.insert(std::pair<string, int>(node_id, party_id));
  }
  return res;
}

