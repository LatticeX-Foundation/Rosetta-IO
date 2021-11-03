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

#include <string.h>
#include <string>
#include "io/internal/logger.h"
#include "io/internal/helper.h"
using namespace std;

namespace rosetta {
namespace io {

/**
 * This class for packing msg_id and real_data, with a total len
 */
class simple_buffer {
 public:
  simple_buffer(const string& id, const char* data, uint64_t length, const string& node_id) {
    len_ = sizeof(uint64_t) + sizeof(uint8_t) + id.size() + length;
    uint8_t id_len = sizeof(uint8_t) + id.size();
    buf_ = new char[len_];
    memset(buf_, 0, len_);
    memcpy(buf_, (const char*)&len_, sizeof(uint64_t));
    memcpy(buf_ + sizeof(uint64_t), (const char*)&id_len, sizeof(uint8_t));
    memcpy(buf_ + sizeof(uint64_t) + sizeof(uint8_t), (const char*)id.data(), id.size());
    memcpy(buf_ + sizeof(uint64_t) + sizeof(uint8_t) + id.size(), data, length);
    string hex_string = get_hex_buffer(buf_, len_);
    log_audit << "all send data to " << node_id << ": " << hex_string;
  }

  ~simple_buffer() {
    delete[] buf_;
  }

 public:
  char* data() {
    return buf_;
  }

  const char* data() const {
    return buf_;
  }
  
  uint64_t len() {
    return len_;
  }

 private:
  uint64_t len_ = 0;
  char* buf_ = nullptr;
};

} // namespace io
} // namespace rosetta
