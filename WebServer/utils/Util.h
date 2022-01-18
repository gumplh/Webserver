#pragma once
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#define MaxBuff 1024
int socket_bind_listen(int port);
int setNoBlocking(int fd);
void setNodely(int fd);  //关闭Negal算法
int readn(int fd, std::string &buff);
int writen(int fd, std::string &buff);
int writen(int fd, char *buff, int size);
int geteventfd();
int ShutdownWR(int fd);
int ShutdwonRDWR(int fd);