#ifndef _SOCKET_
#define _SOCKET_
#include"base/noncopyable.h"
#include<sys/socket.h>
#include"InetAddress.h"


class Socket : noncopyable
{
public:

    explicit Socket(int sockfd);
    Socket(Socket&& socket);    //移动构造
    ~Socket();

    //获取描述符
    int fd() const
    {
        return sockfd_;
    }

    //绑定socket fd与本地ip地址,端口, 核心调用bind(2),失败则终止程序
    void bindAddress(const InetAddress& localaddr);
    //监听函数
    void listen();
    /*
    接受连接函数,成功时，返回一个非负整数，即接受的套接字的描述符，
    它已设置为non-blocking 和 close-on-exec。出错时，返回 -1，并且 *peeraddr 保持不变
    */
    int accept(InetAddress* peeraddr);
    //关闭连接写的这一半
    void  shutdownWrite();
    // TCP_NODELAY选项可以禁用Nagle算法,禁用Nagle算法，可以避免连续发包出现延迟
    void setTcpNoDelay(bool on);
    //设置地址重复利用
    void setReuseAddr(bool on);
    //设置端口重用，多进程或线程绑定或监听同一端口
    void setReusePort(bool on);
    // TCP keepalive是指定期探测连接是否存在，如果应用层有心跳的话，这个选项不是必需要设置的
    void setKeepAlive(bool on);

    

private:
    const int sockfd_;  //套接字描述符
};

#endif