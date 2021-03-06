#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "HttpData.h"
#include "Mutex.h"
#include "ThreadEventLoopPoll.h"
#include "Util.h"
#include "nocopyable.h"
class Server : nocopyable {
 public:
  Server(EventLoop* loop, int port, int threadnum);
  void start();
  void sent();
  void onRequest();
  void acceptSock();
  void Read();
  static void* add2epoll(void* a);

 private:
  std::shared_ptr<EventLoop> loop_;
  int listenfd_;
  std::shared_ptr<Channel> acceptchannel_;
  static const int MAXFDS = 10000;
  ThreadEventLoopPoll threadpoll_;  //线程池
};