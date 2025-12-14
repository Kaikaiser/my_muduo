#include"InetAddress.h"
#include<string>
#include<strings.h>
#include<cstring>
#include<stdint.h>

// 参数的默认值不管在声明还是定义只能出现一次
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    // 使用bzero进行置0（linux专有） memset也可以置0（也可以为其他值）
    // memset(&addr_, 0, sizeof(addr_));
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    // 这是过时的操作 把字符串形式的 IP（如 "192.168.1.100"）转成网络字节序
    // addr_.sin_addr.s_addr = inet_addr(ip.c_str()); 
    ::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);  // 字符串形式的 IP 地址 → 网络字节序 供socket接口使用

}

std::string InetAddress::toIp() const
{
    // addr_  ip->字符串
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); // 网络字节序的二进制地址 -> 字符串形式的 IP 地址
    return buf;
}
std::string InetAddress::toIpPort() const
{
    // ip:port组合
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    // 用strlen获取字符数组长度 到'\0'为止的长度 统计实际字符数
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    // buf + end 指向结尾的 \0 位置。从这里写，相当于“在末尾继续写” 就是插入
    sprintf(buf + end , ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

