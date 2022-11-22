#include"Socket.h"
#include<unistd.h>
#include"base/Logging.h"
#include<linux/tcp.h>
#include"sockets.h"

using namespace std;

Socket::Socket(int sockfd)
    : sockfd_(sockfd)
{
    
}

Socket::Socket(Socket&& socket)
    : sockfd_(socket.sockfd_)
{
    
}

Socket::~Socket()
{
    if(close(sockfd_) < 0)
    {
        LOG_SYSERR << "Socket::~Socket";
    }
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    int ret = bind(sockfd_, sockets::sockaddr_cast(&localaddr.getSockAddrInet()), sizeof(localaddr));
    if (ret < 0)
    {
        LOG_SYSFATAL << "Socket::bindAddress";
    }
}



void Socket::listen()
{
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0)
    {
        LOG_SYSFATAL << "Socket::listen";
    }
}

int Socket::accept(InetAddress* addr)
{
    socklen_t addrlen = sizeof(*addr);
    int connfd = accept4(sockfd_, sockets::sockaddr_cast(&addr->getSockAddrInet()), &addrlen, SOCK_NONBLOCK |  SOCK_CLOEXEC);
    if(connfd < 0)
    {
        int  saveErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch (saveErrno)
        {
            case EAGAIN :
            case ECONNABORTED :
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE: //打开的文件描述符过多
                errno = saveErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG_FATAL << "unexpected error of accept " << saveErrno;
                break;
            default:
                LOG_FATAL << "unknown error of accept " << saveErrno;
                break;
        }
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (shutdown(sockfd_, SHUT_WR) < 0)
  {
    LOG_SYSERR << "Socket::shutdownWrite";
  }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1: 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on) 
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_,IPPROTO_TCP, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP,  SO_KEEPALIVE, &optval, sizeof(optval));
}

