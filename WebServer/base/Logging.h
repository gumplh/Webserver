#pragma once
#include <time.h>
#include <fstream>
#include <iostream>
#include "config.h"
#define MAXLEN 1024

std::string getTime() {
  time_t now = time(0);
  tm* ltm = localtime(&now);
  char time[30];
  sprintf(time, "%d-%d-%d-%d-%d-%d:", 1900 + ltm->tm_year, 1 + ltm->tm_mon,
          ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_min);
  return time;
}
class Logging {
 public:
  Logging(std::string path)
      : path_(path),

        next_(0),
        ptrs(&buff[next_]) {
    std::cout << path_ << std::endl;
    fout.open(path_, std::ios::out | std::ios::app);
    if (!fout.is_open()) {
      perror("fstream");
    }
  }
  ~Logging() {
    fout << (*ptrs) << std::endl;
    ;
    fout.close();
  }
  void operator<<(const std::string s) {
    fout << s << std::endl;
    if ((*ptrs).size() > MAXLEN) {
      fout << (*ptrs);
      next_ = (next_ + 1) % buffnum_;
      ptrs->clear();
      ptrs = &buff[next_];
      fout.flush();
    }
    *ptrs += getTime() + s + "\r\n";
  }

 private:
  std::string* ptrs;
  std::ofstream fout;
  std::string path_;
  int next_;
  static const int buffnum_ = 2;
  std::string buff[buffnum_];
};
