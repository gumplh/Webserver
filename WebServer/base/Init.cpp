#include "Init.h"
#include "Logging.h"
#include "config.h"
#include "myNanoLog.h"
void InitAll() {
  WebserverConfigInstance->InitConfig("../server.yaml");
  nanolog::initialize(nanolog::NonGuaranteedLogger(10), "./tmp", "nanolog", 1);
}