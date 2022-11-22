#ifndef _INETADDRESS_
#define _INETADDRESS_
#include<arpa/inet.h>
#include"base/StringArg.h"


class InetAddress
{
public:

    explicit InetAddress(uint16_t portArg = 0);
    InetAddress(const StringArg& ip, uint16_t portArg);
    InetAddress(const sockaddr_in& addr)
        : addr_(addr){ }

    //将addr_转换成IP形式
    std::string toIp() const;
    //将addr_转换成IP与端口的形式
    std::string toIpPort() const;
    //返回网际地址addr_
    const struct sockaddr_in& getSockAddrInet() const
    {
        return addr_;
    }
    struct sockaddr_in& getSockAddrInet()
    {
        return addr_;
    }
    //设置网际地址addr_的值
    void setSockAddrInet(const sockaddr_in& addr)
    {
        addr_ = addr;
    }
    //返回网络字节序的32位整数IP
    uint32_t ipNetEndian() const
    {
        return addr_.sin_addr.s_addr;
    }
    //返回网络字节序的端口
    uint16_t portNetEndian() const
    {
        return addr_.sin_port;
    }

    static void fromIpPort(const char* ip, uint16_t port, sockaddr_in* addr);

private:
    struct sockaddr_in addr_;
};


#endif