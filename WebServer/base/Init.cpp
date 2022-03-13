#include "Init.h"
#include "Logging.h"
#include "config.h"
extern Logging Log;
void InitAll() {
  WebserverConfigInstance->InitConfig("../server.yaml");
  Log.setPath(WebserverConfigInstance->GetLogPath());
}