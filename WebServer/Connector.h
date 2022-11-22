#ifndef _CONNECTOR_
#define _CONNECTOR_
#include"base/noncopyable.h"
#include"EventLoop.h"
#include<memory>
#include<functional>
#include"InetAddress.h"

class Channel;

class Connector :noncopyable, public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void(int sockfd)> NewConnetionCallback;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    //注册连接 成功后的回调函数
    void setNewConnectionCallback(const NewConnetionCallback& cb)
    {
        newConnetionCallback_ = cb;
    }
    //发起连接
    void start();
    //重新启动连接
    void restart();
    //停止连接
    void stop();
    //返回服务端地址
    InetAddress getServerAddr() const
    {
        return serverAddr_;
    }



private:
    
    enum States
    {
        Disconnected,
        Connecting,
        Connected
    };

    //设置连接状态
    void setState(States s)
    {
        state_ = s;
    }
    //被start()调用，发起连接
    void startInLoop();
    //被stop()调用，断开连接
    void stopInLoop();
    //被startInLoop()调用，发起连接
    void connect();
    //连接成功后调用，初始化通道Channel_
    void connecting(int sockfd);
    //可写时的回调
    void handleWrite();
    //产生错误时的回调
    void handleError();
    //采用back-off策略重连，即重连时间逐渐延长，0.5s, 1s, 2s, ...直至30s
    void retry(int sockfd);
    //将channel_从poller中移除关注，并把channel_置空
    int removeAndResetChannel();
    //被removeAndResetChannel()调用，将channel_置空
    void resetChannel();

    bool connect_;  //连接状态
    EventLoop* loop_;   //所属EventLoop
    States state_;  //状态
    int retryDelayMs_;  //重连延迟时间
    InetAddress serverAddr_;    //服务端地址
    std::unique_ptr<Channel> channel_;  //connector对应的通道Channel
    NewConnetionCallback newConnetionCallback_; //连接成功的回调

    static const int MaxRetryDelayMs;   //最大重连延迟时间
    static const int InitRetryDelayMs;  //初始状态，连接不上，0.5秒后重连
};


#endif