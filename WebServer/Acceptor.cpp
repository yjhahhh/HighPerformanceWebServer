#include"Acceptor.h"
#include<fcntl.h>
#include"sockets.h"
#include"base/Logging.h"

using namespace std;

Acceptor::Acceptor(EventLoop* loop, const InetAddress&addr, bool reuseport)
    : listening_(false), loop_(loop), 
    idelFd_(open("/dev/null", O_RDONLY | O_CLOEXEC)),
    acceptSocket_(sockets::createNonblockingOrDie()),
    acceptChannel_(loop, acceptSocket_.fd())
{
    assert(idelFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(addr);
    acceptChannel_.setReadCallback(bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    sockets::close(idelFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ =  true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(newConneptCallback_)
        {
            newConneptCallback_(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
    else
    {
        LOG_SYSERR << "in Accept::handleRead";
        if(EMFILE)
        {
            //打开的文件描述符过多
            ::close(idelFd_);
            idelFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idelFd_);
            idelFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
