#include "Logging.h"

std::string getTime() {
  time_t now = time(0);
  tm* ltm = localtime(&now);
  char time[30];
  sprintf(time, "%4d-%02d-%02d %02d:%02d:%02d:", 1900 + ltm->tm_year,
          1 + ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
          ltm->tm_min);
  return time;
}

Logging::Logging(std::string path) : path_(path), next_(0), ptrs(&buff[next_]) {
  std::cout << path_ << std::endl;
  fout.open(path_, std::ios::out | std::ios::app);
  if (!fout.is_open()) {
    perror("fstream");
  }
}
void Logging::setPath(std::string path) {
  path_ = path;
  fout.open(path_, std::ios::out | std::ios::app);
  if (!fout.is_open()) {
    perror("fstream");
  }
  next_ = 0;
  ptrs = &buff[next_];
}
Logging::~Logging() {
  fout << (*ptrs) << std::endl;
  fout.close();
}
void Logging::operator<<(const std::string s) {
  fout << getTime() << s << std::endl;
  if ((*ptrs).size() > MAXLEN) {
    fout << (*ptrs);
    next_ = (next_ + 1) % buffnum_;
    ptrs->clear();
    ptrs = &buff[next_];
    fout.flush();
  }
  *ptrs += getTime() + s + "\r\n";
}
Logging Log;