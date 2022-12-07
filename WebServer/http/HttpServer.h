#ifndef _HTTPSERVER_
#define _HTTPSERVER_
#include"../base/noncopyable.h"
#include"../TcpServer.h"
#include"HttpParser.h"
#include<unordered_set>
#include<boost/circular_buffer.hpp>

class HttpServer : noncopyable
{
public:
    typedef std::shared_ptr<HttpParser> ParserPtr;
    typedef std::map<TcpConnectionPtr, ParserPtr> ConnectionParsers;
    typedef TcpServer::ThreadInitCallback ThreadInitCallback;

    HttpServer(EventLoop* loop, const InetAddress& listenAddr, int idleSeconds);

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

private:

    typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;

    class Entry
    {
    public:
        explicit Entry(const WeakTcpConnectionPtr& conn)
            : conn_(conn)
        {
        }
        ~Entry()
        {
            TcpConnectionPtr conn = conn_.lock();   //提升为share_ptr
            if(conn)
            {
                conn->shutdown();
            }
        }

        WeakTcpConnectionPtr conn_;
    };
    typedef std::shared_ptr<Entry> EntryPtr;
    typedef std::weak_ptr<Entry> WeakEntryPtr;
    typedef std::unordered_set<EntryPtr> Bucket;    //存放剩余时间相同的Entry
    typedef boost::circular_buffer<Bucket> WeakConnectionList;  //时间轮

    class HttpContext
    {
    public:
        HttpContext(const WeakEntryPtr& weakEntryPtr, const TcpConnectionPtr& conn)
            : weakEntryPtr_(weakEntryPtr), httpParser_(conn)
        {
        }

        WeakEntryPtr weakEntryPtr_;
        HttpParser httpParser_;
    };

public:
    //定时器到时踢掉空闲的连接
    void onTimer(WeakConnectionList& buckets);
    //线程初始化函数
    void threadFunc(EventLoop* loop, int idleSeconds);

private:
    TcpServer server_;
};

#endif