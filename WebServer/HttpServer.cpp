#include"HttpServer.h"
#include"base/Logging.h"
#include<assert.h>
#include"EventLoop.h"

using namespace std;


HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr)
    : server_(loop, listenAddr, "HttpServer")
{
    server_.setConnectionCallback(bind(&HttpServer::onConnection, this, placeholders::_1));
    server_.setMessageCallback(bind(&HttpServer::onMessage,this, placeholders::_1, placeholders::_2, placeholders::_3));

}

void HttpServer::start()
{
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << conn->peerAddr().toIpPort() << (conn->connected() ? " ON" : " OFF");
    if(conn->connected())
    {
        assert(parsers_.find(conn) == parsers_.end());
        parsers_[conn] = make_shared<HttpParser>();
    }
    else
    {
        assert(parsers_.find(conn) != parsers_.end());
        parsers_.erase(conn);
    }

}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
{
    ParserPtr& parser = parsers_[conn];
    string message = buffer->retrieveAllAsString();
    LOG_INFO << conn->peerAddr().toIpPort() << " : " << message;
    if(parser->hasData())
    {
        parser->appendInput(message);
    }
    else
    {
        parser->newInput(message);
    }
}

void HttpServer::threadFunc(EventLoop* loop)
{
    loop->loop();
}