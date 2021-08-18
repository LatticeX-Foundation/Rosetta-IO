
## Network Topology Configuration
  Network topology is defined in json format. Here is a example.
```json
{
    "NODE_INFO": [
        {
            "NAME": "PartyA(p0)",
            "HOST": "127.0.0.1",
            "PORT": 11121,
            "NODE_ID": "p0"
        },
        {
            "NAME": "PartyB(p1)",
            "HOST": "127.0.0.1",
            "PORT": 12144,
            "NODE_ID": "p1"
        },
        {
            "NAME": "PartyC(p2)",
            "HOST": "127.0.0.1",
            "PORT": 13169,
            "NODE_ID": "p2"
        },
        {
            "NAME": "PartyI(p9)",
            "HOST": "127.0.0.1",
            "PORT": 19361,
            "NODE_ID": "p9"
        }
    ],
    "DATA_NODES": [
        "p0",
        "p1",
        "p2",
        "p9"
    ],
    "COMPUTATION_NODES": {
        "p0": 0,
        "p1": 1,
        "p2": 2
    },
    "RESULT_NODES": [
        "p0",
        "p1",
        "p2",
        "p9"
    ]
}
```
- **`NODE_INFO`**: containing all the information of all the nodes in the network. Include `NAME`, `HOST`, `PORT` and `NODE_ID`. `NODE_ID` is required for all the nodes and other fields are optional.
- **`DATA_NODES`**: containing all the `NODE_ID` of all the nodes owing `DATA_ROLE`.
- **`COMPUTATION_NODES`**: containing all the `NODE_ID` of all the nodes owing `COMPUTATION_ROLE`.
- **`RESULT_NODES`**: containing all the `NODE_ID` of all the nodes owing `RESULT_ROLE`.


## Interface Introduction
  To pass values, serveral C structs are defined. Let us have a look at the structs.

### C Structs

#### NodeIDVec
```cpp
typedef struct {
   int node_count;
   char** node_ids;
}NodeIDVec;
```
- **node\_count**: the count of node ids the struct contains.
- **node\_ids**: a pointer pointing to a array containing pointers which point to the address where the node id stores. Each node id is stored in a C string.
From the members of the struct, we can deduce this struct is to store a array of node ids, just as vector in C++ STL.

#### NodeIDPair
```cpp
typedef struct {
   char* node_id;
   int party_id;
} NodeIDPair;
``` 
- **node\_id**: NODE\_ID of NODE, stored in C string.
- **party\_id**: `PARTY_ID` of NODE, differing `COMPUTATION NODE`.
The struct stores node id and its corresponding party id, just as pair in C++ STL.

#### NodeIDMap
```cpp
typedef struct {
   int node_count;
   NodeIDPair **pairs;
} NodeIDMap;
```
- **node\_count**: the count of node ids the struct contains.
- **pairs**: a pointer pointing to array containing pointers which point to the address where NodeIDPair stores.
The struct stores the node id and its corresponding party id, just as map in C++ STL.


Next, we will introduce C functions
### C Functions
#### CreateInternalChannel
```cpp
IChannel* CreateInternalChannel(const char* task_id, 
                                const char* node_id, 
                                const char* config_str, 
                                error_callback error_cb);
typedef void(*error_callback)(const char* current_node_id, 
                              const char* peer_node_id, 
                              int errorno, 
                              const char* errormsg, 
                              void* user_data);
```
This function is used to create a task level Channel. `IChannel` is a channel handler, just as FILE in standard C library.
- **task_id**, identity of the channel. Rosetta-IO maintains a look-up table storing task id and its corresponding channel. If there is no channel labeled with the same task id, a new channel will be created and it will be recorded in the look-up table.
- **node_id**, id of the current node.
- **config\_str**, network topology configuration. It is in `JSON` format.
- **error_callback**, callback function registered in Rosetta-IO. When network error occurs, this function will be called.  
- **current\_node\_id**: the node id of current node
- **peer\_node\_id**: the node id of the peer node
- **errorno**: errorno denotes the error occured
- **errormsg**: errormsg describes the error occured in detail
- **user\_data**: some data users defined

#### DestroyInternalChannel
```cpp
void DestroyInternalChannel(IChannel* channel);
```
This function is used to destroy a task level channel.
- **channel**, channel handler, returned by CreateInternalChannel, is to be destroyed.

### C++ Interface
#### Send/Recv
```cpp
  int64_t Send(const char* node_id, 
               const char* data_id, const char* data, 
               uint64_t length, int64_t timeout=-1);
  int64_t Recv(const char* node_id, 
               const char* data_id, char* data, 
               uint64_t length, int64_t timeout=-1);
```
The two functions are used to send/receive messages to/from other nodes.
- **node_id**, NODE\_ID of NODE, to which data will be sent to, or from which data will be received.
- **data_id**, identity of data
- **data**, the buffer to send data or receive data
- **length**, the buffer size
- **timeout**, timeout to send/receive message

#### GetCurrentNodeID/GetDataNodeIDs/GetComputationNodeIDs/GetResultNodeIDs/GetConnectionNodeIDs
```cpp
  const char* GetCurrentNodeID();
  const NodeIDVec* GetDataNodeIDs();
  const NodeIDMap* GetComputationNodeIDs();
  const NodeIDVec* GetResultNodeIDs();
  const NodeIDVec* GetConnectedNodeIDs();
```
- **GetCurrentNodeID**, get NODE\_ID of the current node
- **GetDataNodeIDs**, get NODE\_IDs of nodes owning DATA\_ROLE
- **GetComputationNodeIDs**, get NODE\_IDs and PARTY\_IDs of nodes owning COMPUTATION\_ROLE
- **GetResultNodeIDs**, get NODE\_IDs of nodes owning RESULT\_ROLE
- **GetConnectedNodeIDs**, get NODE\_IDs of nodes which have established connection with the current node

### Encode & Decode Functions
#### string/vector/map encode/decode
```cpp
const char* encode_string(const string& str);
string decode_string(const char* pointer);
const NodeIDVec* encode_vector(const vector<string>& vec);
vector<string> decode_vector(const NodeIDVec* pointer);
const NodeIDMap* encode_map(const map<string, int>& m);
map<string, int> decode_map(const NodeIDMap* pointer);
```
- **encode_string**, encode C++ standard string to C string
- **decode_string**, decode C string to C++ standard string
- **encode_vector**, encode C++ STL vector<string> to NodeIDVec
- **decode_vector**, decode NodeIDVec to C++ STL vector<string>
- **encode_map**, encode C++ STL map<string, int> to NodeIDMap
- **decode_map**, decode NodeIDMap to C++ STL map<string, int>

