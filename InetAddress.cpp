#include"InetAddress.h"
#include<string>
#include<string.h>
#include<cstring>
// 参数的默认值不管在声明还是定义只能出现一次
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    // 使用bzero进行置0（linux专有） memset也可以置0（也可以为其他值）
    // memset(&addr_, 0, sizeof(addr_));
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); 

}

std::string InetAddress::toIp() const
{
    // addr_
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
std::string InetAddress::toIpPort() const
{
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
uint16_t InetAddress::toPort() const
{

}

