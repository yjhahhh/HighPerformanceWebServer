#ifndef _TCPSERVER_
#define _TCPSERVER_
#include"base/noncopyable.h"
#include<functional>
#include"map"
#include"TcpConnection.h"
#include<string>
#include"InetAddress.h"
#include<atomic>


class EventLoop;
class Acceptor;
class EventLoopThreadPool;




class TcpServer : noncopyable
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg);
    ~TcpServer();

    const std::string& hostport() const
    {
        return hostport_;
    }
    const std::string name() const
    {
        return name_;
    }

    //设置线程数量
    void setThreadNum(int numThreads);
    //设置线程初始回调
    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        threadInitCallback_ = cb;
    }
    //如果没有监听, 就启动服务器(监听)
    void start();
    //设置连接到来或者连接关闭回调函数
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connectionCallback_ = cb;
    }
    //设置消息到来回调函数
    void setMessageCallback(const MessageCallback& cb)
    {
        messageCallback_ = cb;
    }
    //设置写完成回调函数
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        writeCompleteCallback_ = cb;
    }



private:
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    void newConnection(int sockfd, const InetAddress& peerAddr);

    void removeConnection(const TcpConnectionPtr& conn);

    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    int nextConnId_;    //下一个连接Id
    EventLoop* loop_;   //所属EventLoop
    const std::string hostport_;    //服务端口
    const std::string name_;    //服务名
    std::atomic_bool started_;  //是否开启
    std::unique_ptr<Acceptor> acceptor_;    //监听器
    std::unique_ptr<EventLoopThreadPool> threadPool_;    //事件循环线程池
    ConnectionCallback connectionCallback_; //连接回调
    MessageCallback messageCallback_;   //完成数据读取回调
    WriteCompleteCallback writeCompleteCallback_;   //数据发送完成回调
    ThreadInitCallback threadInitCallback_; //IO线程池中的线程在进入事件循环前，会回调用此函数
    ConnectionMap connections_; //连接列表

};

#endif