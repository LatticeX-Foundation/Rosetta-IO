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
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>
using namespace std;
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

typedef void(*error_callback)(const char*, const char*, int, const char*, void*);

/// Channel interface definition
/// the functionality is sending a message to peer and 
/// receiving a message from peer
class IChannel {
public:
  virtual ~IChannel() {};

   /**
   * @brief Set error callback for error handle.
   * @param error_cb error callback to handle error.
   * @note Should set callback from python to c++, Rosetta internal should not set error callback.
  */
  virtual void SetErrorCallback(error_callback error_cb)= 0;

  /**
   * @brief Recv receive a message from message queue， for the target node (blocking for timeout microseconds, default waiting forever)
   * @param node_id target node id for message receiving.
   * @param id identity of a message, could be a task id or message id.
   * @param data buffer to receive a message.
   * @param length data length expect to receive
   * @param timeout timeout to receive a message.
   * @return 
   *  return message length if receive a message successfully
   *  0 if peer is disconnected  
   *  -1 if it gets a exception or error
  */
  virtual int64_t Recv(const char* node_id, const char* id, char* data, uint64_t length, int64_t timeout=-1) = 0;

  /**
   * @brief Send send a message to target node
   * @param node_id target node id for message receiving
   * @param id identity of a message, could be a task id or message id.
   * @param data buffer to send
   * @param length data length expect to send
   * @param timeout timeout to receive a message.
   * @return 
   *  return length of data has been sent if send a message successfully
   *  -1 if gets exceptions or error
  */
  virtual int64_t Send(const char* node_id, const char* id, const char* data, uint64_t length, int64_t timeout=-1) = 0;

  /**
    *@brief flush all data to be sent
    */
  virtual void Flush() = 0;

  /**
   * @brief get node id of all the data nodes
   * @return
   * return node id of all the data nodes
  */
 virtual const NodeIDVec* GetDataNodeIDs() = 0;

  /**
   * @brief get node id and party id of all the computation nodes
   * @return
   * return node id and party id of all the computation nodes
   * 
  */
  virtual const NodeIDMap* GetComputationNodeIDs() = 0;

  /**
   * @brief get node id of all the result nodes
   * @return
   * return node id of all the result nodes
  */
  virtual const NodeIDVec* GetResultNodeIDs() = 0;
  /**
   * @brief get node id of the current node
   * @return
   * return node id of the current node
  */
  virtual const char* GetCurrentNodeID() = 0;

  /**
   * @brief get node id of all the nodes establishing connection with the current node
   * @return
   * return node id of all the nodes establishing connection with the current node
  */
  virtual const NodeIDVec* GetConnectedNodeIDs() = 0;
};// IChannel


