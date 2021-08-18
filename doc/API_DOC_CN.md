## 网络拓扑配置
  网络拓扑结果是用json格式定义的，以下是一个例子。
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
- **`NODE_INFO`**: 包含网络中所有节点的所有信息，包括`NAME`, `HOST`, `PORT`和`NODE_ID`. `NODE_ID`对所有节点来说是必需的，其它字段是可选的。
- **`DATA_NODES`**: 包含拥有`数据角色`的所有节点的`NODE_ID`。
- **`COMPUTATION_NODES`**: 包含拥有`计算角色`的所有节点的`NODE_ID`。
- **`RESULT_NODES`**: 包含拥有`结果节点`的所有节点的`NODE_ID`。


## 接口简介
  为了传递参数，定义了几个C结构体。接下来将先介绍C结构体。

### C结构体

#### NodeIDVec
```cpp
typedef struct {
   int node_count;
   char** node_ids;
}NodeIDVec;
```
- **node\_count**: 结构体中包含的`节点ID`的数量。
- **node\_ids**: 指向指针数组的指针，数组中的每个指针都指向以C字符串方式存储的`节点ID`。
根据以上结构体成员的定义可知，该结构体用来存储`节点ID`组成的数组，就像C++ STL中的vector一样。

#### NodeIDPair
```cpp
typedef struct {
   char* node_id;
   int party_id;
} NodeIDPair;
``` 
- **node\_id**: 用C字符串存储的`节点ID`。
- **party\_id**: 节点的`PARTY_ID`，用来区分 `计算节点`.
这个结构体存储单个`节点ID`和对应的`PARTY_ID`，就像C++ STL中的pair一样。

#### NodeIDMap
```cpp
typedef struct {
   int node_count;
   NodeIDPair **pairs;
} NodeIDMap;
```
- **node\_count**: 结构体中`节点ID`的数量。
- **pairs**: 指向一个指针数组，数组中的每个指针执行一个NodeIDPair。
这个结构体存储多个`节点ID`和对应的`PARTY_ID`，就像C++ STL中的map一样。


接下来，将介绍C函数
### C函数
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
这个函数用来创建一个任务级别的通道。`IChannel`是一个通道句柄，就像`FILE`在标准C库一样。
- **task_id**, 任务ID。Rosetta-IO维护了一个存储任务ID及其对应的通道的查找表。如果查找表中没有存储相应的任务ID和通道，新通道将被建立并在查找表中存储。
- **node_id**, 当前节点的`节点ID`。
- **config\_str**, 网络拓扑结构配置，是`JSON`格式的。
- **error_callback**, 将在Rosetta-IO中注册的回调函数。网络错误发生时，该函数会被回调。
- **current\_node\_id**: 当前节点的`节点ID`
- **peer\_node\_id**: 对端节点的`节点ID`
- **errorno**: 错误码
- **errormsg**: 错误消息
- **user\_data**: 用户自定义数据

#### DestroyInternalChannel
```cpp
void DestroyInternalChannel(IChannel* channel);
```
这个函数用来销毁通道。
- **channel**, 将被销毁的通道句柄，是`CreateInternalChannel`的返回值

#### Send/Recv
```cpp
  int64_t Send(const char* node_id, 
               const char* data_id, const char* data, 
               uint64_t length, int64_t timeout=-1);
  int64_t Recv(const char* node_id, 
               const char* data_id, char* data, 
               uint64_t length, int64_t timeout=-1);
```
这两个函数用来发送消息给其它节点或者从其它节点接收消息。
- **node_id**, 节点的`节点ID`，消息将被发送给该节点，或者将从该节点接收数据。
- **data_id**, 数据ID
- **data**, 用来发送或者接收数据的缓冲区
- **length**, 缓冲区大小
- **timeout**, 发送数据或者接收数据的超时时间

### C++函数
#### GetCurrentNodeID/GetDataNodeIDs/GetComputationNodeIDs/GetResultNodeIDs/GetConnectionNodeIDs
```cpp
  const char* GetCurrentNodeID();
  const NodeIDVec* GetDataNodeIDs();
  const NodeIDMap* GetComputationNodeIDs();
  const NodeIDVec* GetResultNodeIDs();
  const NodeIDVec* GetConnectedNodeIDs();
```
- **GetCurrentNodeID**, 获取当前节点的`节点ID`
- **GetDataNodeIDs**, 获取所有拥有`数据角色`的节点的`节点ID`
- **GetComputationNodeIDs**, 获取所有拥有`计算角色`的节点的`节点ID`和`PARTY_ID`
- **GetResultNodeIDs**, 获取所有拥有`结果角色`的节点的`节点ID`
- **GetConnectedNodeIDs**, 获取所有与当前节点建立网络连接的节点的`节点ID`

### 编码和解码函数
#### string/vector/map encode/decode
```cpp
const char* encode_string(const string& str);
string decode_string(const char* pointer);
const NodeIDVec* encode_vector(const vector<string>& vec);
vector<string> decode_vector(const NodeIDVec* pointer);
const NodeIDMap* encode_map(const map<string, int>& m);
map<string, int> decode_map(const NodeIDMap* pointer);
```
- **encode_string**, 将C++标准字符串编码成C字符串
- **decode_string**, 将C字符串解码成C++标准字符串
- **encode_vector**, 将C++ STL vector<string>编码成NodeIDVec
- **decode_vector**, 将NodeIDVec解码成C++ STL vector<string>
- **encode_map**, 将C++ STL map<string, int>编码成NodeIDMap
- **decode_map**, 将NodeIDMap解码成C++ STL map<string, int>


