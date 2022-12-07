#include"TcpServer.h"
#include"EventLoop.h"
#include"EventLoopThreadPool.h"
#include"Acceptor.h"
#include"base/Logging.h"
#include"sockets.h"


using namespace std;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& addr, const string& nameArg)
    : started_(false), nextConnId_(1), loop_(loop),
    hostport_(addr.toIpPort()), name_(nameArg),
    acceptor_(make_unique<Acceptor>(loop, addr,  true)),
    threadPool_(make_unique<EventLoopThreadPool>(loop, nameArg)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback)
{
    acceptor_->setNewConneptCallback(bind(&TcpServer::newConnection, this, placeholders::_1, placeholders::_2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";
    for(auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(!started_.exchange(true, std::memory_order_acq_rel))
    {
        threadPool_->start(threadInitCallback_);
        assert(!acceptor_->listening());
        loop_->runInLoop(bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf),  "-%s#%d", hostport().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;
    InetAddress localAddr = sockets::getLocalAddr(sockfd);
    TcpConnectionPtr conn = make_shared<TcpConnection>(ioLoop, connName,  sockfd, localAddr, peerAddr);

    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(bind(&TcpServer::removeConnection,  this, placeholders::_1));

    conn->getLoop()->runInLoop(bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConntionInLoop [" << name_ << "] - connection " << conn->getName();
    size_t n = connections_.erase(conn->getName());
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(bind(&TcpConnection::connectDestroyed, conn));
}