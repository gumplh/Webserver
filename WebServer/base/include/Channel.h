#pragma once
#include <string.h>
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include "nocopyable.h"

class EventLoop;
class HttpData;
typedef std::function<void()> CallBack;
class Channel : nocopyable {
 public:
  Channel(std::shared_ptr<EventLoop> loop);
  Channel(std::shared_ptr<EventLoop> loop, int fd);
  void setHolder(std::shared_ptr<HttpData> holder);
  void handleEvent();
  __uint32_t get_event() { return event_; }
  void expired();
  std::shared_ptr<HttpData> getHolder();
  void setRevent(const __uint32_t &e);
  void setCallRead(CallBack &&cb);
  void setCallWrite(CallBack &&cb);
  void setEvent(const __uint32_t &e);
  int getFd() { return fd_; }

 private:
  int fd_;
  CallBack callRead_;
  CallBack callWrite_;
  __uint32_t event_;  //注册的事件
  std::shared_ptr<EventLoop> loop_;

  __uint32_t revent_;                 //返回的事件
  std::shared_ptr<HttpData> holder_;  //方便找到持有Channel的对象
};