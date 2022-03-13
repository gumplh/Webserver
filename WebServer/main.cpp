#include "EventLoop.h"
#include "Init.h"
#include "Server.h"
#include "myNanoLog.h"
using namespace std;
int main(int argc, char* argv[]) {
  InitAll();
  EventLoop loop_;
  Server server_(&loop_, WebserverConfigInstance->GetPort(), 4);
  server_.start();
  LOG_INFO << "start Webserver...";
  loop_.loop();
  return 0;
}