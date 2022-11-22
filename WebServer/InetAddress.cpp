#include"InetAddress.h"
#include<string.h>
#include<endian.h>
#include"base/Logging.h"

using namespace std;

InetAddress::InetAddress(uint16_t portArg)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr =  htobe32(INADDR_ANY);
    addr_.sin_port = htobe16(portArg);
}

InetAddress::InetAddress(const StringArg& ip, uint16_t port)
{
    bzero(&addr_, sizeof(addr_));
    fromIpPort(ip.c_str(), port, &addr_);
}

void InetAddress::fromIpPort(const char* ip, uint16_t port,sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htobe16(port);
    if(inet_pton(AF_INET, ip, addr) <= 0)
    {
        LOG_SYSERR << "InetAddress::formIpPort";
    }
}

string InetAddress::toIp() const
{
    char buf[32];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

string InetAddress::toIpPort() const
{
    char host[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, host, sizeof(host));
    char buf[32];
    sprintf(buf, "%s:%u", host, be16toh(addr_.sin_port));
    return buf;
}

