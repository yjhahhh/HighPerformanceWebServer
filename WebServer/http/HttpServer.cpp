#include"HttpServer.h"
#include"../base/Logging.h"
#include<assert.h>
#include"../EventLoop.h"

using namespace std;

static const size_t HighWaterMark = 128 * 1024;  //128K字节


HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr, int idleSeconds)
    : server_(loop, listenAddr, "HttpServer")
{
    server_.setConnectionCallback(bind(&HttpServer::onConnection, this, placeholders::_1));
    server_.setMessageCallback(bind(&HttpServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));
    server_.setThreadInitCallback(bind(&HttpServer::threadFunc, this, placeholders::_1, idleSeconds));
}

void HttpServer::start()
{
    server_.start();
}

//高水位回调
void handleHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
    //简单地关闭conn的读事件监听即可
    conn->stopRead();
}
//低水位回调
void handleWriteComplete(const TcpConnectionPtr& conn)
{
    //恢复conn对读事件的监听
    conn->startRead();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << conn->peerAddr().toIpPort() << (conn->connected() ? " ON" : " OFF");
    if(conn->connected())
    {
        EntryPtr entryPtr = make_shared<Entry>(conn);
        WeakConnectionList* buckets = boost::any_cast<WeakConnectionList>(conn->getLoop()->getMutableContext());
        buckets->back().insert(entryPtr);   //计时
        WeakEntryPtr weakEntry(entryPtr);
        conn->setContext(HttpContext(weakEntry, conn));
        conn->setHighWaterMarkCallback(handleHighWaterMark, HighWaterMark); //设置高水位回调
        conn->setWriteCompleteCallback(handleWriteComplete);
    }

}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
{
    HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());
    string message = buffer->retrieveAllAsString();
    LOG_INFO << conn->peerAddr().toIpPort() << " : \n" << message;
    if(context->httpParser_.hasData())
    {
        context->httpParser_.appendInput(message);
    }
    else
    {
        context->httpParser_.newInput(message);
    }
    if(!context->httpParser_.process())
    {
        conn->shutdown();
    }
    else
    {
        EntryPtr entryPtr(context->weakEntryPtr_.lock());
        if(entryPtr)
        {
            WeakConnectionList* buckets = boost::any_cast<WeakConnectionList>(conn->getLoop()->getMutableContext());
            buckets->back().insert(entryPtr);   //重新计时
        }
    }
}

void HttpServer::onTimer(WeakConnectionList& buckets)
{
    buckets.push_back(Bucket());   //会把首部的Bucket析构
}

void HttpServer::threadFunc( EventLoop* loop, int idleSeconds)
{
    WeakConnectionList buckets(idleSeconds);
    buckets.resize(idleSeconds);
    loop->setContext(buckets);
    loop->runEvery(1.0, bind(&HttpServer::onTimer, this, buckets));
}