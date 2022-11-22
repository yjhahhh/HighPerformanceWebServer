#ifndef _SOCKETSFUNC_
#define _SOCKETFUNC_
#include<arpa/inet.h>

namespace sockets
{

//创建一个非阻塞的套接字，如果创建失败，则终止程序
int createNonblockingOrDie();
//将文件描述符sockfd设置为非阻塞模式
void setNonBlockAndCloseOnExec(int sockfd);
//获取sockfd套接字的本地地址
sockaddr_in getLocalAddr(int sockfd);
//获取sockfd套接字的对等方地址
sockaddr_in getPeerAddr(int sockfd);
//判断是否是自连接
bool isSelfConnect(int sockfd);

int connect(int sockfd, const sockaddr_in& addr);

int getSocketError(int sockfd);

void close(int sockfd);

const sockaddr* sockaddr_cast(const sockaddr_in*);
sockaddr* sockaddr_cast(sockaddr_in* );


}
#endif