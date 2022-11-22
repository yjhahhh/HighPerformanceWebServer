#include"TcpConnection.h"
#include"EventLoop.h"
#include"Socket.h"
#include"Channel.h"
#include"base/Logging.h"
#include"Buffer.h"
#include"sockets.h"

using namespace std;


void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->localAddr().toIpPort() << " -> "
            << conn->peerAddr().toIpPort() << " is "
            << (conn->connected() ? " UP" : " DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
    buf->retrieveAll();
}


TcpConnection::TcpConnection(EventLoop* loop, const string& nameArg, int sockfd,
                             const InetAddress& localAddr, const InetAddress& peerAddr)
    : reading_(false), loop_(loop), name_(nameArg), state_(Connecting),
    socket_(make_unique<Socket>(sockfd)),
    channel_(make_unique<Channel>(loop, sockfd)),
    localAddr_(localAddr), peerAddr_(peerAddr)
{
    //通道可读事件到来的时候，回调TcpConnection::handleRead，_1是事件发生时间
    channel_->setReadCallback(bind(&TcpConnection::handleRead, this, placeholders::_1));
    channel_->setWriteCallback(bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd = " << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this << " fd = " << channel_->fd();
}

int TcpConnection::fd() const
{
    return channel_->fd();
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(Connecting == state_);
    setState(Connected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if(Connected == state_)
    {
        setState(DisConnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::send(string message)
{
    StringPiece msg(message);
    send(msg);
}

void TcpConnection::send(StringPiece& message)
{
    if(Connected == state_)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::sendInLoop(StringPiece& message)
{
    loop_->assertInLoopThread();
    size_t nwrote = 0;
    size_t remaining = message.size();
    bool faultError = false;
    if(DisConnected == state_)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    //发送缓冲区为空
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::send(socket_->fd(), message.data(), message.size(), 0);
        if(nwrote >= 0)
        {
            remaining = message.size() - nwrote;
            if(remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if(errno == EWOULDBLOCK)    //发送缓冲区已满且为非阻塞
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    //对端已发FIN/RST分节 表明tcp连接发生致命错误(faultError为true)
                    faultError = true;
                }
            }
        }
    }
    assert(remaining <= message.size());
    //没有故障, 并且还有待发送数据, 可能是发送太快, 对方来不及接收
    if(!faultError && remaining > 0)
    {
        size_t oldlen = outputBuffer_.readableBytes();  //Buffer中待发送数据量
        if(oldlen + remaining > highWaterMark_  //Buffer及当前要发送的数据量之和 超 高水位
        && oldlen < highWaterMark_  //单独的Buffer中待发送数据量未超过高水位 
        && highWaterMarkCallback_)
        {
            loop_->queueInLoop(bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
        }

        message.remove_prefix(nwrote);
        outputBuffer_.append(message);
        if(!channel_->isWriting())
        {
            //如果没有在监听通道写事件, 就使能通道写事件
            channel_->enableWriting();
        }
    }

}

//从输入缓存inputBuffer_读取数据, 交给回调messageCallback_处理
void TcpConnection::handleRead(const Timestamp& receiveTime)
{
    loop_->assertInLoopThread();
    int saveErrno = 0;
    ssize_t n = inputBUffer_.readFd(socket_->fd(), &saveErrno);
    if(n > 0)
    {
        messageCallback_(shared_from_this(), &inputBUffer_, receiveTime);
    }
    else if(n == 0)
    {
        //关闭连接
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if(channel_->isWriting())
    {
        ssize_t n = ::send(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes(), 0);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                    loop_->queueInLoop(bind(writeCompleteCallback_, shared_from_this()));
                if (DisConnecting == state_)
                {
                    shutdownInLoop();
                } 
                
            }
        }
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }
    else
    {
        LOG_TRACE << "Connection fd = " << channel_->fd() << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd();
    assert(Connected == state_ || DisConnecting == state_);
    setState(DisConnected);
    channel_->disableAll();
    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);

    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    char msg[1024];
    LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_r(err, msg, sizeof(msg));
}

void TcpConnection::shutdown()
{
    if(Connected == state_)
    {
        setState(DisConnected);
        loop_->runInLoop(bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }

}

void TcpConnection::forceClose()
{
    if(DisConnecting == state_ || Connected == state_)
    {
        setState(DisConnecting);
        loop_->queueInLoop(bind(&TcpConnection::forceCloseInLoop,this));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if(DisConnecting == state_ || Connected == state_)
    {
        loop_->runAfter(seconds, bind(&TcpConnection::forceClose, this));
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if(DisConnecting == state_ || Connected == state_)
    {
        handleClose();
    }
}

void TcpConnection::startRead()
{
    loop_->runInLoop(bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
    loop_->assertInLoopThread();
    if(!reading_ || !channel_->isReading())
    {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead()
{
    loop_->runInLoop(bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if(reading_ || channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}