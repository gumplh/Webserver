#include "EventLoop.h"
#include "Logging.h"
#include "Server.h"
#include "config.h"
using namespace std;

int main(int argc, char* argv[]) {
  WebserverConfigInstance->InitConfig("../server.yaml");
  Logging Log{WebserverConfigInstance->GetLogPath()};
  Log << "start Webserver...";
  EventLoop loop_;
  Server server_(&loop_, WebserverConfigInstance->GetPort(), 4);
  server_.start();
  loop_.loop();
  return 0;
}