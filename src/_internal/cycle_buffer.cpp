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
#include "io/internal/cycle_buffer.h"
#include "io/internal/logger.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assert.h>
#include <unistd.h>
using namespace std;

namespace rosetta {
namespace io {

cycle_buffer::~cycle_buffer() {
  delete[] buffer_;
  buffer_ = nullptr;
}

void cycle_buffer::reset() {
  is_full_ = false;
  is_empty_ = true;
  r_pos_ = 0;
  w_pos_ = 0;
  remain_space_ = n_;
}

bool cycle_buffer::can_read(uint64_t length) {
  std::unique_lock<std::mutex> lck(mtx_);
  return (n_ - remain_space_ >= length);
}

bool cycle_buffer::can_remove(double t) {
  if (
    (remain_space_ == n_) // no datas
    && (timer_.elapse() > t) // no visits in t seconds
  ) {
    return true;
  }
  return false;
}
/////////////////////////////////////////
int64_t cycle_buffer::peek(char* data, uint64_t length) {
  timer_.start();
  {
    unique_lock<mutex> lck(mtx_);
    cv_.wait(lck, [&]() {
      if (n_ - remain_space_ >= length)
        return true;
      cout << "can not peek" << endl;
      return false;
    });
  }
  timer_.start();
  {
    unique_lock<mutex> lck(mtx_);
    if (r_pos_ >= w_pos_) {
      if (r_pos_ <= n_ - length) {
        memcpy(data, buffer_ + r_pos_, length);
      } else {
        uint64_t first_n = n_ - r_pos_;
        memcpy(data, buffer_ + r_pos_, first_n);
        memcpy(data + first_n, buffer_, length - first_n);
      }
    } else {
      memcpy(data, buffer_ + r_pos_, length);
    }
    cv_.notify_all();
  }
  timer_.start();
  return length;
}

bool cycle_buffer::can_read() {
  unique_lock<mutex> lck(mtx_);
  //log_info << "can read remain data:" << n_ - remain_space_;
  if (n_ - remain_space_ > sizeof(uint64_t) + sizeof(uint8_t)) {
    uint64_t len = 0;
    if (r_pos_ < w_pos_) {
      len = *(uint64_t*)(buffer_ + r_pos_);
    } else {
      char dlen[sizeof(uint64_t)];
      if (r_pos_ <= n_ - sizeof(uint64_t)) {
	      memcpy(dlen, buffer_ + r_pos_, sizeof(uint64_t));
      } else {
	      memcpy(dlen, buffer_ + r_pos_, n_ - r_pos_);
	      memcpy(dlen + n_ - r_pos_, buffer_, sizeof(uint64_t) - (n_ - r_pos_));
      }
        len = *(uint64_t*)dlen;
    }
    //log_info << "remain data:" << n_ - remain_space_ << ", data size:" << len;
    if (n_ - remain_space_ >= len)
      return true;
  }
  return false;
}

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
static char get_hex_char(char p) {
  if (p >= 0 && p <= 9) {
    return '0' + p;
  } else {
    return 'A' + (p - 10);
  }
}
static string get_hex_str(char p) {
  char c1 = get_hex_char((unsigned char)(p) >> 4);
  char c2 = get_hex_char(p & 0x0F);
  char tmp[3] = {0};
  tmp[0] = c1;
  tmp[1] = c2;
  return string(tmp);
}
static void print_str(const char* p, int len, const string& node_id) {
  string str = "";
  for (int i = 0; i < len; i++) {
    str += get_hex_str(*p);
    p++;
  }
  log_audit << "all recv data from " << node_id << ":" << str;
}

int64_t cycle_buffer::read(string& id, string& data) {
  if (n_ - remain_space_ > sizeof(uint64_t) + sizeof(uint8_t)) {
    unique_lock<mutex> lck(mtx_);
    uint64_t len = 0;
    string tmp;
    if (r_pos_ < w_pos_) {
      len = *(uint64_t*)(buffer_ + r_pos_);
      if (n_ - remain_space_ >= len) {
	      tmp.resize(len);
	      memcpy(&tmp[0], buffer_ + r_pos_, len);
      }
    } else {
      char data_len[sizeof(uint64_t)];
      if (r_pos_ <= n_ - sizeof(uint64_t)) {
        memcpy(data_len, buffer_ + r_pos_, sizeof(uint64_t));
      } else {
        uint64_t remain_len = n_ - r_pos_;
        memcpy(data_len, buffer_ + r_pos_, remain_len);
        memcpy(data_len + remain_len, buffer_, sizeof(uint64_t) - remain_len);
      }
      len = *(uint64_t*)data_len;
      if (n_ - remain_space_ >= len) {
	      tmp.resize(len);
        if (n_ - r_pos_ >= len) {
          memcpy(&tmp[0], buffer_ + r_pos_, len);
        } else {
	        memcpy(&tmp[0], buffer_ + r_pos_, n_ - r_pos_);
	        memcpy((char*)&tmp[0] + n_ - r_pos_, buffer_, len - (n_ - r_pos_));
        }
      }
    }
    if (tmp.size() > sizeof(uint64_t) + sizeof(uint8_t)) {
      uint8_t len2 = *(uint8_t*)((char*)&tmp[0] + sizeof(uint64_t));
      id.resize(len2 - sizeof(uint8_t));
      memcpy(&id[0], (char*)&tmp[0] + sizeof(uint64_t) + sizeof(uint8_t), id.size());
      data.resize(len - sizeof(uint64_t) - len2);
      memcpy(&data[0], (char*)&tmp[0] + sizeof(uint64_t) + len2, data.size());
      r_pos_ = (r_pos_ + len) % n_;
      remain_space_ += len;
      print_str(tmp.data(), tmp.size(), node_id);
      return tmp.size();
    }
  }
  return 0;
}

int64_t cycle_buffer::read(char* data, uint64_t length) {
  timer_.start();
  {
    do {
      unique_lock<mutex> lck(mtx_);
      cv_.wait_for(lck, std::chrono::milliseconds(1000), [&]() {
        if (n_ - remain_space_ >= length)
          return true;
        return false;
      });
      if (n_ - remain_space_ >= length)
        break;
    } while (true);
  }
  timer_.start();
  {
    unique_lock<mutex> lck(mtx_);
    if (r_pos_ >= w_pos_) {
      if (r_pos_ <= n_ - length) {
        memcpy(data, buffer_ + r_pos_, length);
        r_pos_ += length;
      } else {
        uint64_t first_n = n_ - r_pos_;
        memcpy(data, buffer_ + r_pos_, first_n);
        memcpy(data + first_n, buffer_, length - first_n);
        r_pos_ = length - first_n;
      }
    } else {
      memcpy(data, buffer_ + r_pos_, length);
      r_pos_ += length;
    }
    remain_space_ += length;
    cv_.notify_all();
  }
  timer_.start();
  return length;
}

void cycle_buffer::realloc(uint64_t length) {
  unique_lock<mutex> lck(mtx_);
  if (remain_space_ >= length) {
    return;
  }

  if (remain_space_ < length) {
    uint64_t new_n = n_ * ((length / n_) + 2); // at least 2x
    log_debug << "buffer can not write. expected:" << length << ", actual:" << remain_space_
              << ". will expand from " << n_ << " to " << new_n ;

    char* newbuffer_ = new char[new_n];
    uint64_t havesize = size();
    if (w_pos_ > r_pos_) {
      memcpy(newbuffer_, buffer_ + r_pos_, havesize);
    } else {
      uint64_t first_n = n_ - r_pos_;
      memcpy(newbuffer_, buffer_ + r_pos_, first_n);
      if (havesize - first_n > 0) {
        memcpy(newbuffer_ + first_n, buffer_, havesize - first_n);
      }
    }
    n_ = new_n;
    remain_space_ = n_ - havesize;
    r_pos_ = 0;
    w_pos_ = havesize;
    delete[] buffer_;
    buffer_ = newbuffer_;
    newbuffer_ = nullptr;
  }
}

// data --> buffer_
int64_t cycle_buffer::write(const char* data, uint64_t length) {
  timer_.start();
  {
    realloc(length);
    unique_lock<mutex> lck(mtx_);
    cv_.wait(lck, [&]() {
      if (remain_space_ >= length)
        return true;
      cout << "never enter here. can not write" << endl;
      return false;
    });
  }
  timer_.start();
  {
    unique_lock<mutex> lck(mtx_);
    if (w_pos_ >= r_pos_) {
      if (w_pos_ <= n_ - length) {
        memcpy(buffer_ + w_pos_, data, length);
        w_pos_ += length;
      } else {
        uint64_t first_n = n_ - w_pos_;
        memcpy(buffer_ + w_pos_, data, first_n);
        memcpy(buffer_, data + first_n, length - first_n);
        w_pos_ = length - first_n;
      }
    } else {
      memcpy(buffer_ + w_pos_, data, length);
      w_pos_ += length;
    }
    remain_space_ -= length;
    //log_info << "write remain data:" << n_ - remain_space_;
    cv_.notify_all();
  }
  timer_.start();
  return length;
}

} // namespace io
} // namespace rosetta
