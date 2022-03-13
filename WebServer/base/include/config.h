#pragma once
#include <string>

class WebserverConfig {
 public:
  bool InitConfig(std::string config_path);
  static WebserverConfig* GetIntance();
  int GetPort() { return port; }
  std::string GetLogPath() { return logpath; }

 public:
  WebserverConfig(const WebserverConfig&) = delete;
  WebserverConfig(WebserverConfig&&) = delete;
  WebserverConfig& operator=(const WebserverConfig) = delete;
  WebserverConfig& operator=(WebserverConfig&&) = delete;

 private:
  WebserverConfig() = default;
  ~WebserverConfig() = default;

 private:
  int port;
  std::string logpath;
};
#define WebserverConfigInstance WebserverConfig::GetIntance()