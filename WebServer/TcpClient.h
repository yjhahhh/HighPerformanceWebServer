#ifndef _TCPCLIENT_
#define _TCPCLIENT_
#include"base/noncopyable.h"
#include"TcpConnection.h"



class  Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable
{
public:

    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& nameArg);
    ~TcpClient();

    //发起连接
    void connect();
    //连接已建立的情况下关闭连接
    void disConnect();
    //连接还未建立时停止连接
    void stop();
    //设置连接处于重连状态
    void enableRetry()
    {
        retry_ = true;
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
    //新连接建立时的回调
    void newConnection(int scokfd);
    //连接断开时的回调
    void removeConnection(const TcpConnectionPtr& conn);

    const std::string& name() const
    {
        return name_;
    }


private:

    bool retry_;    //是否重连
    bool connect_;  //是否连接
    EventLoop* loop_;   //所属EventLoop
    int nextConnId_;    //下一个连接Id
    const std::string name_;    //客户端名
    ConnectorPtr connector_;    //连接器，主动发起连接
    ConnectionCallback connectionCallback_; //连接回调
    MessageCallback messageCallback_;   //完成数据读取回调
    WriteCompleteCallback writeCompleteCallback_;   //写数据完成回调
    TcpConnectionPtr connection_;   //Tcp连接

};


#endif