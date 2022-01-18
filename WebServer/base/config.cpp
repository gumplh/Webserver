
#include "config.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

WebserverConfig* WebserverConfig::GetIntance(){
    static WebserverConfig config;
    return &config;
}
bool WebserverConfig::InitConfig(std::string config_path){
    YAML::Node config = YAML::LoadFile(config_path);
    port = config["server"]["port"].as<int>();
    logpath = config["log"]["path"].as<std::string>();
}