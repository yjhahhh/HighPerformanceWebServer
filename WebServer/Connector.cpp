#include"Connector.h"
#include"base/Logging.h"
#include"sockets.h"
#include"Channel.h"
#include<errno.h>

using namespace std;

const int Connector::MaxRetryDelayMs = 30 * 1000;
const int Connector::InitRetryDelayMs = 500;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : connect_(false), loop_(loop), state_(Disconnected),
    retryDelayMs_(InitRetryDelayMs), serverAddr_(serverAddr)
{
    LOG_DEBUG << "Connector::ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "Connect::dtor[" << this << "]";
}


void Connector::start()
{
    connect_ == true;
    loop_->runInLoop(bind(&Connector::startInLoop, this));

}

 void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(Disconnected == state_);
    if(connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop()
{
    connect_ = false;
    loop_->runInLoop(bind(&Connector::stopInLoop, this));

}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if(Connecting == state_)
    {
        setState(Disconnected);
        int sockfd = removeAndResetChannel();   //将通道从poller中移除关注，并将channel置空
        retry(sockfd);  //这里并非要重连，只是调用close(sockfd);
    }

}

void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int saveErrno = ret == 0 ? 0 : errno;
    switch (saveErrno)
    {
    case 0:
    case EINPROGRESS:   //非阻塞套接字，未连接成功返回码是EINPROGRESS表示正在连接
    case EINTR: //被中断
    case EISCONN:   //连接成功
        connecting(sockfd);
        break;
    case EAGAIN:    //重新尝试
    case EADDRINUSE:    //地址已在使用中
    case EADDRNOTAVAIL: //不能分配本地地址,一般在端口不够用的时候会出现
    case ECONNREFUSED:  //连接被服务器拒绝
    case ENETUNREACH:   //无法传送数据包至指定的主机
        retry(sockfd);  //重连
        break;          
 /*
 用户尝试连接到广播地址，但没有启用套接字广播标志
 或连接请求由于本地防火墙规则而失败
*/
    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:  //传递的地址中没有正确的地址族其sa_family字段
    case EALREADY:  //套接字是非阻塞的，以前的连接尝试尚未完成
    case EBADF: //sockfd不是有效的打开文件描述符
    case EFAULT:    //套接字结构地址在用户地址之外空间
    case ENOTSOCK:  //文件描述符sockfd未引用套接字
        LOG_SYSERR << "connect error in Connector::startInLoop " << saveErrno;
        close(sockfd);  //不能重连
        break;
    default :
        LOG_SYSERR << "Unexpected error in Connector::startInLoop " << saveErrno;
        sockets::close(sockfd);
        break;

    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(Disconnected);
    retryDelayMs_ = InitRetryDelayMs;
    connect_ =  true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    setState(Connecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    //设置可写回调函数
    channel_->setWriteCallback(bind(&Connector::handleWrite, this));
    //设置发生错误回调
    channel_->setErrorCallback(bind(&Connector::handleError, this));
    channel_->tie(shared_from_this());
    channel_->enableWriting();  //让poller关注可写事件

}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    //不能在这里重置channel_，因为正在调用Channel::handleEvent
    loop_->queueInLoop(bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << state_;
    if(Connecting == state_)
    {
        int sockfd = removeAndResetChannel();
        //sockfd可写不代表连接一定建立成功
        int err = sockets::getSocketError(sockfd);
        if(err)
        {
            //有错误
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " <<  err;
            retry(sockfd);    //重连
        }
        else if(sockets::isSelfConnect(sockfd))
        {
            //自连接
            LOG_WARN << "COnnector::handleWrite - SelfConnect";
            retry(sockfd);
        }
        else
        {
            //连接成功
            setState(Connected);
            if(connect_)
            {
                //回调
                newConnetionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        assert(Disconnected == state_);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError";
    assert(Connecting == state_);
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    char msg[1024] {0};
    LOG_TRACE << "SO_ERROR = " << err << " " << strerror_r(err, msg, sizeof(err));
    retry(sockfd);  //重连
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(Disconnected);
    if(connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
                << " in " << retryDelayMs_ << " milliseconds. ";
        //注册一个定时操作，重连
        loop_->runAt(retryDelayMs_ / 1000.0, bind(&Connector::startInLoop, this));
        retryDelayMs_ = min(retryDelayMs_ * 2, MaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}