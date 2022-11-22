#ifndef _TCPCONNECTION_
#define _TCPCONNECTION_
#include"base/noncopyable.h"
#include<memory>
#include<functional>
#include"Buffer.h"
#include"base/Timestamp.h"
#include<string>
#include"InetAddress.h"
#include"base/StringPiece.h"


class EventLoop;
class Socket;
class Channel;
class TcpConnection;
class Buffer;

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
//数据已被读取
typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;


//Tcp连接
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:

    TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd,
                const InetAddress& localaddr, const InetAddress& peerAddr);
    ~TcpConnection();

    int fd() const;
    EventLoop* getLoop() const
    {
        return loop_;
    }
    const std::string& getName() const
    {
        return name_;
    }
    const InetAddress& localAddr() const
    {
        return localAddr_;
    }
    const InetAddress& peerAddr() const
    {
        return peerAddr_;
    }
    bool connected() const
    {
        return Connected == state_;
    }
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb)
    {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb)
    {
        closeCallback_ = cb;
    }
    Buffer* inputBuffer()
    {
        return &inputBUffer_;
    }
    Buffer* outputBuffer()
    {
        return &outputBuffer_;
    }

    //发送消息
    void send(StringPiece& message);
    void send(std::string message);
    //关闭写半连接
    void shutdown();
    //强制关闭连接
    void forceClose();
    //强制延时关闭连接
    void forceCloseWithDelay(double seconds);
    //
    void startRead();
    void stopRead();
    //当TcpServer接受一个新连接时调用
    void connectEstablished();
    //连接断开时调用
    void connectDestroyed();

private:

    enum StateE
    {
        DisConnected,
        Connecting,
        Connected,
        DisConnecting
    };

    //通道读事件到来时回调
    void handleRead(const Timestamp& receiveTime);
    //通道写事件到来时回调
    void handleWrite();
    //通道出现错误事件时回调
    void handleError();
    //处理关闭连接事件
    void handleClose();
    //loop_线程中排队发消息
    void sendInLoop(StringPiece& message);
    //loop线程中排队关闭写连接
    void shutdownInLoop();
    //loop线程中排队关闭连接
    void forceCloseInLoop();
    //loop线程在排队开始监听读事件
    void startReadInLoop();
    //loop线程在排队开始监听读事件
    void stopReadInLoop();
    

    void setState(StateE s)
    {
        state_ = s;
    }

    bool reading_;  //是否在监听读事件
    EventLoop* loop_;   //所属EventLoop
    size_t highWaterMark_;  //高水位阈值
    std::string name_;  //连接名
    StateE state_;  //连接状态
    std::unique_ptr<Socket> socket_;    //套接字
    std::unique_ptr<Channel> channel_;  //通道
    InetAddress localAddr_; //本地地址
    InetAddress peerAddr_;  //对端地址
    ConnectionCallback connectionCallback_; //连接回调
    MessageCallback messageCallback_;   //完成数据读取回调
    WriteCompleteCallback writeCompleteCallback_;   //写完成回调
    HighWaterMarkCallback highWaterMarkCallback_;   //高水位回调
    CloseCallback closeCallback_;   //关闭连接回调
    Buffer inputBUffer_;    //输入缓冲区
    Buffer outputBuffer_;   //输出缓冲区

};


//默认的连接回调
void defaultConnectionCallback(const TcpConnectionPtr& conn);
//默认的完成数据读取回调
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);


#endif