#include <arpa/inet.h>
#include <string.h>
#include "SsmsNetAddress.h"
#include "Base/SsmsLogStream.h"

using namespace ssms::nw;

SsmsNetAddress::SsmsNetAddress(const std::string &ip, int port, bool ipv4)
 : ipv4_(ipv4), ip_(ip), port_(std::to_string(port))
{

}

void SsmsNetAddress::SetIp(const std::string &ip, int port, bool ipv4)
{
    ipv4_ = ipv4;
    ip_ = ip;
    port_ = std::to_string(port);
}

void SsmsNetAddress::GetSockAddr(struct sockaddr *addr)
{
    if (!addr)
    {
        LOG_ERROR << "addr is nullptr";
        return;
    }
    if (ipv4_)
    {
        struct sockaddr_in *addr_in = reinterpret_cast<struct sockaddr_in *>(addr);
        memset(addr_in, 0, sizeof(struct sockaddr_in));
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = ::htons(::atoi(port_.c_str()));
        if (::inet_pton(AF_INET, ip_.c_str(), &addr_in->sin_addr.s_addr) <= 0)
        {
            LOG_ERROR << "inet_pton convert str ip to interger error";
            return;
        }
    }
    else
    {
        struct sockaddr_in6 *addr_in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);
        memset(addr_in6, 0, sizeof(struct sockaddr_in6));
        addr_in6->sin6_family = AF_INET6;
        addr_in6->sin6_port = ::htons(::atoi(port_.c_str()));
        if (::inet_pton(AF_INET6, ip_.c_str(), &addr_in6->sin6_addr) <= 0)
        {
            LOG_ERROR << "inet_pton convert str ip to interger error";
            return;
        }
    }
}

std::string SsmsNetAddress::GetStringIp() const
{
    return ip_;
}

std::string SsmsNetAddress::GetPort() const
{
    return port_;
}