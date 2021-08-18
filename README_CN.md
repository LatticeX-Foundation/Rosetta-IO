[![github license](https://img.shields.io/badge/license-LGPLv3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.en.html)

--------------------------------------------------------------------------------

[English Edition](./README.md)

## 概述
  Rosetta-IO是Rosetta的一个基础组件，对Rosetta提供IO服务。Rosetta-IO中的通道一旦被建立，就能够发送数据给其它节点或者从其它节点接收数据。在Rosetta-IO，`节点`通过`节点ID`来区分，它是TCP连接的一端。每个节点都有全局的`(主机, 端口)`元组，`主机`是`IP地址`或者`域名`，`端口`是TCP监听端口。
  
  Rosetta-IO支持`多数据源`。它定义了节点的3种角色，分别是`数据角色`，`计算角色`和`结果角色`。`数据角色`意味着节点输入数据或者模型是合法的。拥有`计算角色`的节点可以参与`隐私计算`。`隐私计算`的结果只能存储在`结果节点`。每个节点可以有一种或者更多种角色。Rosetta-IO根据节点角色建立网络拓扑结构。网络拓扑结构由3个子网络构成，即`输入网络`，`计算网络`，`输出网络`。`输入网络`由`数据节点`和`计算节点`组成，`计算网络`由`计算节点`组成，`输出网络`由`计算节点`和`结果节点`组成。

  Rosetta-IO支持`多任务`。其中的`通道`是任务基本的句柄，每个接口都是基于一个具体任务的。在Rosetta-IO，节点之间创建的网络连接在任务之间是复用的，任何两个节点之间建立的网络连接都只有一条。


## 编译和安装
  按照以下步骤编译和安装Rosetta-IO
```bash
$ git clone --recurse https://github.com/LatticeX-Foundation/Rosetta-IO.git
$ cd Rosetta-IO
$ export install_path=~/.local/rosetta-io
$ mkdir -p build && cd build
$ cmake ../ -DCMAKE_INSTALL_PREFIX=${install_path}
$ core_num=$(nproc)
$ make -j${core_num} && make install
```


### 示例
  这里给出一个示例来展示Rosetta-IO的使用方法。
  假设用三方，分别是P0, P1和P2，需要比较他们的网络拓扑配置文件。他们可能写如下的程序来完成这项工作。(check_config_json.cpp)
```cpp
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <io/internal_channel.h>
#include <io/channel.h>
#include <io/channel_decode.h>
using namespace std;
int main(int argc, char* argv[]) {
  if (argc != 3) {
  printf("argc != 3\n");
  return -1;
  }
  const char* file_name = argv[1];
  const char* node_id = argv[2];
  string config_str = "";
  char buf[1024];
  FILE* fp = fopen(file_name, "r");
  if (fp == nullptr) {
    printf("open file %s error", file_name);
    return -1;
  }
  while (fgets(buf, sizeof(buf), fp) != nullptr) {
    config_str += string(buf);
  }
  fclose(fp);
  printf("config:%s", config_str.c_str());
  
  IChannel* channel = ::CreateInternalChannel("test", node_id, config_str.c_str(), nullptr);
  vector<string> connected_nodes = ::decode_vector(channel->GetConnectedNodeIDs());
  int config_str_size = config_str.size();
  const char* data_id = "config str";
  for (int i = 0; i < connected_nodes.size(); i++) {
    channel->Send(connected_nodes[i].c_str(), data_id, (const char*)&config_str_size, sizeof(int));
    channel->Send(connected_nodes[i].c_str(), data_id, config_str.data(), config_str_size);
    printf("send data to %s\n", connected_nodes[i].c_str());
  }
  for (int i = 0; i < connected_nodes.size(); i++) {
    int data_size = 0;
    channel->Recv(connected_nodes[i].c_str(), data_id, (char*)&data_size, sizeof(int));
    string data(data_size, 0);
    channel->Recv(connected_nodes[i].c_str(), data_id, &data[0], data_size);
    printf("recv data from %s, size:%d\n", connected_nodes[i].c_str(), data_size);
  }
  ::DestroyInternalChannel(channel);
  return 0;
}
``` 
P0和P1使用下面的配置文件，即p0p1.json。
```json
{
    "NODE_INFO": [
        {
            "NAME": "PartyA(P0)",
            "HOST": "127.0.0.1",
            "PORT": 11121,
            "NODE_ID": "P0"
        },
        {
            "NAME": "PartyB(P1)",
            "HOST": "127.0.0.1",
            "PORT": 12144,
            "NODE_ID": "P1"
        },
        {
            "NAME": "PartyC(P2)",
            "HOST": "127.0.0.1",
            "PORT": 13169,
            "NODE_ID": "P2"
        }
    ],
    "DATA_NODES": [
        "P0",
        "P1",
        "P2"
    ],
    "COMPUTATION_NODES": {
        "P0": 0,
        "P1": 1,
        "P2": 2
    },
    "RESULT_NODES": [
        "P0",
        "P1",
        "P2"
    ]
}
```
P2使用下面的配置文件，即p2.json。
```json
{
    "NODE_INFO": [
        {
            "NAME": "PartyA(P0)",
            "HOST": "127.0.0.1",
            "PORT": 11121,
            "NODE_ID": "P0"
        },
        {
            "NAME": "PartyB(P1)",
            "HOST": "127.0.0.1",
            "PORT": 12144,
            "NODE_ID": "P1"
        },
        {
            "NAME": "(P2)",
            "HOST": "127.0.0.1",
            "PORT": 13169,
            "NODE_ID": "P2"
        }
    ],
    "DATA_NODES": [
        "P0",
        "P1",
        "P2"
    ],
    "COMPUTATION_NODES": {
        "P0": 0,
        "P1": 1,
        "P2": 2
    },
    "RESULT_NODES": [
        "P0",
        "P1",
        "P2"
    ]
}
```
这两个文件的唯一差别是在p0p1.json中P2的`NAME`是`PartyC(P2)`，而在p2.json中P2的`NAME`是`(P2)`。
这三方按照以下的步骤安装rosetta-io和生成程序`check_config_json`。
```bash
#install_path
$ git clone --recurse https://github.com/LatticeX-Foundation/Rosetta-IO.git
$ cd Rosetta-IO
$ mkdir -p build
$ cd build
$ export install_path=~/.local/rosetta-io
$ cmake ../ -DCMAKE_INSTALL_PREFIX=${install_path}
$ core_num=$(nproc)
$ make -j${core_num} && make install
$ cd ../example
$ g++ check_config_json.cpp -o check_config_json -I${install_path}/include -L${install_path}/lib -lio -Wl,-rpath=${install_path}/lib
```

接下来，这三方各自独立启动程序。
P0如下启动。
```bash
$ ./check_config_json p0p1.json P0
```

P1如下启动。
```bash
$ ./check_config_json p0p1.json P1
```

P2如下启动。
```bash
$ ./check_config_json p2.json P2
```


这三方将在控制终端得到不同的运行结果。
以下是P0得到的结果。
```bash
send data to P1
send data to P2
recv data from P1, size:716
recv data from P2, size:710
```

以下是P1得到的结果。
```bash
send data to P0
send data to P2
recv data from P0, size:716
recv data from P2, size:710
```

以下是P2得到的结果。
```bash
send data to P0
send data to P1
recv data from P0, size:716
recv data from P1, size:716
```


## 文档列表
* [Rosetta-IO用户API](./doc/API_DOC_CN.md)
