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
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

class IChannel;
/**
 * @brief error_callback definition
 * @param current_node_id the node id of current node
 * @param pear_node_id the node id of the pear node
 * @param errorno errorno denotes the error occured
 * @param errormsg errormsg describes the error occured in detail
 * @param user_data some data users defined
*/
typedef void(*error_callback)(const char*current_node_id, const char*pear_node_id, int errorno, const char* errormsg, void* user_data);

/**
 * @brief NodeIDVec has `node_count` elements. Each element is a pointer pointing to a C-style string containing node id. 
*/
typedef struct {
   int node_count;
   char** node_ids;
}NodeIDVec;

/**
 * @brief NodeIDPair has node id and party id in pair.
*/
typedef struct {
   char* node_id;
   int party_id;
} NodeIDPair;

/**
 * @brief NodeIDMap has `node_count` elements. Each element is a pointer pointing to a NodeIDPair structure.
*/
typedef struct {
   int node_count;
   NodeIDPair **pairs;
} NodeIDMap;


   /**
   * @brief Set error callback for error handle.
   * @param channel Channel Handler
   * @param error_cb error callback to handle error.
   * @note Should set callback from python to c++, Rosetta internal should not set error callback.
  */
  void SetErrorCallback(IChannel* channel, error_callback error_cb);

  /**
   * @brief Recv receive a message from message queue， for the target node (blocking for timeout microseconds, default waiting forever)
   * @param channel Channel Handler
   * @param node_id target node id for message receiving.
   * @param data_id identity of a message, could be a task id or message id.
   * @param data buffer to receive a message.
   * @param length data length expect to receive
   * @param timeout timeout to receive a message.
   * @return 
   *  return message length if receive a message successfully
   *  0 if peer is disconnected  
   *  -1 if it gets a exception or error
  */
  int64_t Recv(IChannel* channel, const char* node_id, const char* data_id, char* data, uint64_t length, int64_t timeout=-1);

  /**
   * @brief Send send a message to target node
   * @param channel Channel Handler
   * @param node_id target node id for message receiving
   * @param data_id identity of a message, could be a task id or message id.
   * @param data buffer to send
   * @param length data length expect to send
   * @param timeout timeout to receive a message.
   * @return 
   *  return length of data has been sent if send a message successfully
   *  -1 if gets exceptions or error
  */
  int64_t Send(IChannel* channel, const char* node_id, const char* data_id, const char* data, uint64_t length, int64_t timeout=-1);

  /**
   * @brief get node id of all the data nodes
   * @param channel Channel Handler
   * @return
   * return node id of all the data nodes
  */
  const NodeIDVec* GetDataNodeIDs(IChannel* channel);

  /**
   * @brief get node id and party id of all the computation nodes
   * @param channel Channel Handler
   * @return
   * return node id and party id of all the computation nodes
   * string  indicates node id and int indicates party id
  */
  const NodeIDMap* GetComputationNodeIDs(IChannel* channel);

  /**
   * @brief get node id of all the result nodes
   * @param channel Channel Handler
   * @return
   * return node id of all the result nodes
  */
  const NodeIDVec* GetResultNodeIDs(IChannel* channel);
  /**
   * @brief get node id of the current node
   * @param channel Channel Handler
   * @return
   * return node id of the current node
  */
  const char* GetCurrentNodeID(IChannel* channel);

  /**
   * @brief get node id of all the nodes establishing connection with the current node
   * @param channel Channel Handler
   * @return
   * return node id of all the nodes establishing connection with the current node
  */
  const NodeIDVec* GetConnectedNodeIDs(IChannel* channel);

#ifdef __cplusplus
}
#endif
