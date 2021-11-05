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
#include "io/internal/simple_timer.h"

#include <mutex>
#include <condition_variable>
#include <string>
using namespace std;

namespace rosetta {
namespace io {

struct cycle_buffer {
  uint64_t r_pos_ = 0; // read position
  uint64_t w_pos_ = 0; // write position
  uint64_t n_ = 0; // buffer size
  uint64_t remain_space_ = 0;
  char* buffer_ = nullptr;
  std::mutex mtx_;
  std::condition_variable cv_;

  /// a timer for rm empty <msgid -> buffer>
  SimpleTimer timer_;

  uint64_t size() { return n_ - remain_space_; }
  uint64_t remain_space() const { return remain_space_; }

 public:
  ~cycle_buffer();
  cycle_buffer(uint64_t n) : n_(n), remain_space_(n) { buffer_ = new char[n_]; }
  void reset();

 public:
  // if i can read length size buffer
  bool can_read(uint64_t length);
  bool can_read();
  bool can_remove(double t);

  /**
   * The real data will not be deleted. \n
   * The caller must make sure that can read length size bytes data
   */
  int64_t peek(char* data, uint64_t length);

  /**
   */
  int64_t read(char* data, uint64_t length);
  int64_t read(string& id, string& data, const string& node_id);
  void realloc(uint64_t length);
  int64_t write(const char* data, uint64_t length);
};
} // namespace io
} // namespace rosetta
