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

#include "io/internal/logger.h"
#include "io/internal/rtt_exceptions.h"
#include "io/internal/netutil.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <signal.h>

namespace rosetta {
namespace io {
#ifndef E_TIMEOUT
#define E_ERROR -1
#define E_TIMEOUT -3
#define E_UNCONNECTED -4
#endif

/**
 * The base class of Server/Client
 */
class Socket {

 public:
  Socket();
  virtual ~Socket() = default;

 public:
  static std::string gethostip(std::string hostname = "127.0.0.1");

  // 1
 public:
  int set_nonblocking(int fd, bool value) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
      return errno;
    }
    if (value) {
      return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
  }

  //! Options
 protected:
  bool option_reuseaddr_ = true;
  bool option_reuseport_ = true;

 public:
  bool option_reuseaddr() const { return option_reuseaddr_; }
  bool option_reuseport() const { return option_reuseport_; }

 protected:
  int set_reuseaddr(int fd, int optval);
  int set_reuseport(int fd, int optval);
  int set_sendbuf(int fd, int size);
  int set_recvbuf(int fd, int size);
  int set_send_timeout(int fd, int64_t timeout);
  int set_recv_timeout(int fd, int64_t timeout);
  size_t default_buffer_size_ = 1024 * 1024 * 10;
  size_t default_buffer_size() { return default_buffer_size_; }

 protected:
  int set_linger(int fd);
  int set_nodelay(int fd, int optval);

  //! Helpers
 protected:
  struct timeval gettv(int ms) {
    timeval t;
    t.tv_sec = ms / 1000;
    t.tv_usec = (ms % 1000) * 1000;
    return t;
  }

 protected:
  bool is_ssl_socket_ = false;
};


} // namespace io
} // namespace rosetta
