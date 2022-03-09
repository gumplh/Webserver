#pragma once
#include <time.h>
#include <fstream>
#include <iostream>
#include "config.h"
#define MAXLEN 1024

std::string getTime();
class Logging {
 public:
  Logging() = default;
  Logging(std::string path);
  void setPath(std::string path);
  ~Logging();
  void operator<<(const std::string s);

 private:
  std::string *ptrs;
  std::ofstream fout;
  std::string path_;
  int next_;
  static const int buffnum_ = 2;
  std::string buff[buffnum_];
};