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
