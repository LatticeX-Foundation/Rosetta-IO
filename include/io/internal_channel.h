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

#include "io/channel.h"

// the format of message identity is in hex if disabled
// it can be transformed to binary format to reduce bytes transfered over network
#define DEBUG_MSG_ID 0

#ifdef __cplusplus
extern "C" {
#endif

// interface to create internal channel
/**
 * @brief Interface to create a task level channel
 * @param task_id identity of channel
 * @param node_id define current node
 * @param config_str contain all the information of nodes to build the network topology, 
 *                                          including NODE_INFO, DATA_NODES, COMPUTATION_NODES and RESULT_NODES
 * @param error_cb when a network error occurs, error_cb will be called automatically
 * @return channel handler, a pointer to a channel instance
*/
IChannel* CreateInternalChannel(const char* task_id, const char* node_id, const char* config_str, error_callback error_cb);

/**
 * @brief interface to destroy a channel
 * @param channel a channel to be destroyed
*/
void DestroyInternalChannel(IChannel* channel);


#ifdef __cplusplus
}
#endif
