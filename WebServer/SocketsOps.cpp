#include "sockets.h"
#include"base/Logging.h"
#include<sys/socket.h>
#include<fcntl.h>

int sockets::createNonblockingOrDie()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0)
  {
    LOG_SYSFATAL << "sockets::createNonblockingOrDie";

  }
}

void sockets::setNonBlockAndCloseOnExec(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    flags =fcntl(sockfd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(sockfd, F_SETFD, flags);
}

sockaddr_in sockets::getLocalAddr(int sockfd)
{
    sockaddr_in localaddr;
    socklen_t addrlen = sizeof(localaddr);
    bzero(&localaddr, addrlen);
    if(getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

sockaddr_in sockets::getPeerAddr(int sockfd)
{
    sockaddr_in peeraddr;
    socklen_t addrlen = sizeof(peeraddr);
    bzero(&peeraddr, addrlen);
    if(getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

sockaddr*sockets::sockaddr_cast(sockaddr_in* addr)
{
    return static_cast<sockaddr*>(static_cast<void*>(addr));
}

const sockaddr* sockets::sockaddr_cast(const sockaddr_in* addr)
{
    return static_cast<const sockaddr*>(static_cast<const void*>(addr));
}

/*
自连接是指(sourceIP, sourcePort) = (destIP, destPort)
自连接发生的原因:
客户端在发起connect的时候，没有bind(2)
客户端与服务器端在同一台机器，即sourceIP = destIP，服务器尚未开启，即服务器还没有在destPort端口上处于监听
就有可能出现自连接，这样，服务器也无法启动了
*/
bool sockets::isSelfConnect(int sockfd)
{
    sockaddr_in localaddr = getLocalAddr(sockfd);
    sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr && localaddr.sin_port == peeraddr.sin_port;
}

int sockets::connect(int sockfd, const sockaddr_in& addr)
{
    return ::connect(sockfd, sockaddr_cast(&addr), sizeof(addr));
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof(optval);
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

void sockets::close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOG_SYSERR << "sockets::close";
    }
}