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
using namespace std;

#define PRINT_HEX_SIZE 128

inline char get_hex_char(char p) {
  if (p >= 0 && p <= 9) {
    return '0' + p;
  } else {
    return 'a' + (p - 10);
  }
}

inline string get_hex_buffer(const void* buf, size_t size) {
  char tmp[8] = {0};
  char* p = (char*)buf;
  string s;
  size_t min_size = size < PRINT_HEX_SIZE ? size : PRINT_HEX_SIZE;
  for (size_t i = 0; i < min_size; i++) {
    tmp[0] = get_hex_char((unsigned char)(p[i]) >> 4);
    tmp[1] = get_hex_char(p[i] & 0x0F);
    s.append(tmp);
  }
  if (min_size < size) {
    s.append(3, '.');
  }
  return s;
}