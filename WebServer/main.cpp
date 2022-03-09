#include "EventLoop.h"
#include "Init.h"
#include "Logging.h"
#include "Server.h"
using namespace std;
extern Logging Log;
int main(int argc, char* argv[]) {
  InitAll();
  EventLoop loop_;
  Log << "start Webserver...";
  Server server_(&loop_, WebserverConfigInstance->GetPort(), 2);
  server_.start();
  
  loop_.loop();
  return 0;
}