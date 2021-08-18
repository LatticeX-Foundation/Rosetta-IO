[![github license](https://img.shields.io/badge/license-LGPLv3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.en.html)

--------------------------------------------------------------------------------

[中文版](./README_CN.md)

## Overview
  Rosetta-IO is a building block of Rosetta, providing IO service for Rosetta. Once the channel in Rosetta-IO  is created, it is available to send/receive messages from other nodes. In Rosetta-IO, `NODE` identified by `NODE ID`, is a endpoint of a TCP connection. Each node has unique `(HOST, PORT)` tuple where `HOST` is a `IP ADDRESS` or `DOMAIN NAME` and `PORT` is a `TCP LISTENING PORT`.
  
  Rosetta-IO supports `MULTI DATA SOURCE`. It defines three roles, called `DATA ROLE`, `COMPUTATION ROLE` and `RESULT ROLE`. `DATA ROLE` denotes that inputing data/module from the node is legal. Nodes, owning `COMPUTATION ROLE`, can participate in `PRIVACY COMPUTATION`. The results of `PRIVACY COMPUTATION` can only be stored on nodes with `RESULT ROLE`. Each node can have one or more roles. Rosetta-IO establishes network topoloy according to the roles of nodes. Three subnetworks, namely `INPUT NETWORK`, `COMPUTATION NETWORK` and `OUTPUT NETWORK`, will be built. `INPUT NETWORK` consists of nodes owning `DATA ROLE` and/or `COMPUTATION ROLE`. All of the nodes in `COMPUTATION NETWORK` have `COMPUTATION ROLE`. Nodes with `COMPUTATION ROLE` and nodes with `RESULT_ROLE` establish `OUTPUT NETWORK` together.

  Rosetta-IO supports `MULTI TASKING`. The term `CHANNEL` refers to a task level handler. Every interface provided is based on a specific task. In Rosetta-IO, the connection between any two nodes is reused inter-task and there is only one connection between any two nodes.


## Compile & Install
  Follow the steps below to compile and install Rosetta-IO
```bash
$ git clone --recurse https://github.com/LatticeX-Foundation/Rosetta-IO.git
$ cd Rosetta-IO
$ export install_path=~/.local/rosetta-io
$ mkdir -p build && cd build
$ cmake ../ -DCMAKE_INSTALL_PREFIX=${install_path}
$ core_num=$(nproc)
$ make -j${core_num} && make install
```


### Example
  Here we give a example to demostrate how to use Rosetta-IO.
  Support three parties, called P0, P1 and P2, need to compare their config file on network topology. They may write the following program to finish this task.(check_config_json.cpp)
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
P0 and P1 use the following config file, named p0p1.json.
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
P2 uses the follow config file, named p2.config.
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
The only difference between the two config files is that `NAME` of P2 in p0p1.json is `PartyC(P2)` while `NAME` of P2 in p2.json is `(P2)`.
The three parties follow the steps below to install rosetta-io and generate program `check_config_json`.
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

Next, the three parties start their program respectively.
Here is what P0 does.
```bash
$ ./check_config_json p0p1.json P0
```

Here is what P1 does.
```bash
$ ./check_config_json p0p1.json P1
```

Here is what P2 does.
```bash
$ ./check_config_json p2.json P2
```


The three parties get different results from their console terminals.
Here is what P0 gets.
```bash
send data to P1
send data to P2
recv data from P1, size:716
recv data from P2, size:710
```

Here is what P1 gets.
```bash
send data to P0
send data to P2
recv data from P0, size:716
recv data from P2, size:710
```

Here is what P2 gets
```bash
send data to P0
send data to P1
recv data from P0, size:716
recv data from P1, size:716
```


## Document List
* [Rosetta-IO User API](./doc/API_DOC.md)
