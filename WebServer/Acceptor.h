#ifndef _ACCEPTOR_
#define _ACCEPTOR_
#include"base/noncopyable.h"
#include<functional>
#include"Socket.h"
#include"EventLoop.h"
#include"Channel.h"



class Acceptor : noncopyable
{
public:
    typedef std::function<void(int connfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport);
    ~Acceptor();

    //设置新连接回调
    void setNewConneptCallback(const NewConnectionCallback& cb)
    {
        newConneptCallback_ = cb;
    }

    //监听本地端口
    void listen();
    //当前是否在监听端口
    bool listening() const
    {
        return listening_;
    }

private:
    //处理读事件,接受连接
    void handleRead();

    bool listening_;    //监听状态
    EventLoop* loop_;   //所属的EventLoop
    int idelFd_;    //空闲的文件描述符，用于fd资源不够用时, 可以空一个出来作为新建连接connfd
    Socket acceptSocket_;   //用于接受连接的套接字
    Channel acceptChannel_; //接受连接通道 ，监听connfd
    NewConnectionCallback  newConneptCallback_; //新建 连接的回调


};

#endif