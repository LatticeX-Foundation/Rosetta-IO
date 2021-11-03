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
#include "cc/modules/common/include/utils/rtt_logger.h"
using namespace std;

namespace rosetta {
namespace io {

/**
 * This class for packing msg_id and real_data, with a total len
 */
static char get_hex_char2(char p) {
  if (p >= 0 && p <= 9) {
    return '0' + p;
  } else {
    return 'A' + (p - 10);
  }
}
static string get_hex_str2(char p) {
  char c1 = get_hex_char2((unsigned char)(p) >> 4);
  char c2 = get_hex_char2(p & 0x0F);
  char tmp[3] = {0};
  tmp[0] = c1;
  tmp[1] = c2;
  return string(tmp);
}
static void print_str2(const char* p, int len, const string& node_id) {
  string str = "";
  for (int i = 0; i < len; i++) {
    str += get_hex_str2(*p);
    p++;
  }
  log_audit << "all send data to " << node_id << ":" << str;
}
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
    print_str2(buf_, len_, node_id);
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
