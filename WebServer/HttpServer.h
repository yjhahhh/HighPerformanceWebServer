#ifndef _HTTPSERVER_
#define _HTTPSERVER_
#include"base/noncopyable.h"
#include"TcpServer.h"
#include"HttpParser.h"
#include<map>

class HttpServer : noncopyable
{
public:
    typedef std::shared_ptr<HttpParser> ParserPtr;
    typedef std::map<TcpConnectionPtr, ParserPtr> ConnectionParsers;
    typedef TcpServer::ThreadInitCallback ThreadInitCallback;

    HttpServer(EventLoop* loop, const InetAddress& listenAddr);

    //开启服务器
    void start();

    //完成数据读取的回调，交给解析器解析
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time);
    //新连接到来的回调
    void onConnection(const TcpConnectionPtr& conn);
    //设置线程回调函数
    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        server_.setThreadInitCallback(cb);
    }
    //设置线程数量
    void setThreadNum(int n)
    {
        server_.setThreadNum(n);
    }

    void threadFunc(EventLoop* loop);

private:

    TcpServer server_;
    ConnectionParsers parsers_; //每个连接都有一个http解析器
};

#endif