#include "Init.h"
#include "config.h"
#include "Logging.h"
extern Logging Log;
void InitAll(){
    WebserverConfigInstance->InitConfig("../server.yaml");

    Log.setPath(WebserverConfigInstance->GetLogPath());
}