#include"TcpClient.h"
#include"Connector.h"
#include"EventLoop.h"
#include"base/Logging.h"

using namespace std;

void removeConnectionDtor(EventLoop* loop, const TcpConnectionPtr& conn)
{
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const string& nameArg)
    : retry_(false), connect_(true), loop_(loop),
    nextConnId_(1), name_(nameArg),
    connector_(make_shared<Connector>(loop, serverAddr)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback)
{
    connector_->setNewConnectionCallback(bind(&TcpClient::newConnection, this, placeholders::_1));
    LOG_INFO << "TcpClient::TcpClient [" << name_ << "] - connector " << connector_.get();
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << name_ <<"] - connector " << connector_.get();
    if(connection_)
    {
        assert(loop_ == connection_->getLoop());
        CloseCallback cb = bind(&removeConnectionDtor, loop_, placeholders::_1);
        loop_->runInLoop(bind(&TcpConnection::setCloseCallback, connection_, cb));
        if(connection_.unique())
        {
            connection_->forceClose();
        }
    }
    else
    {
        connector_->stop();
    }
}

void TcpClient::connect()
{
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to " << connector_->getServerAddr().toIpPort();

    connect_ = true;
    connector_->start();
}

void TcpClient::disConnect()
{
    connect_ = false;
    if(connection_)
    {
        connection_->shutdown();
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr = connection_->peerAddr();
    char buf[32];
    snprintf(buf, sizeof(buf), "%s#%d", peerAddr.toIpPort(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;
    InetAddress localAddr = connection_->localAddr();

    connection_ = make_shared<TcpConnection>(loop_, connName, sockfd, localAddr, peerAddr);
    connection_->setConnectionCallback(connectionCallback_);
    connection_->setMessageCallback(messageCallback_);
    connection_->setWriteCompleteCallback(writeCompleteCallback_);
    connection_->setCloseCallback(bind(&TcpClient::removeConnection, this, placeholders::_1));

    connection_->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());
    connection_.reset();
    loop_->queueInLoop(bind(&TcpConnection::connectDestroyed, conn));
    if(retry_ && connect_)
    {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to " << connector_->getServerAddr().toIpPort();
        connector_->restart();  //重连
    }
}