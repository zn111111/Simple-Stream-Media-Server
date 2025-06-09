#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "SsmsSocket.h"
#include "Base/SsmsLogStream.h"

using namespace ssms::nw;

//SOCK_STREAM： tcp
//SOCK_NONBLOCK: 非阻塞
//SOCK_CLOEXEC: 子进程中自动关闭
int SsmsSocket::CreateTcpNonBlockSocket()
{
    return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

//SOCK_DGRAM: udp
int SsmsSocket::CreateUdpNonBlockSocket()
{
    return ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
}

void SsmsSocket::Bind(int fd, const struct sockaddr &addr)
{
    if (::bind(fd, &addr, sizeof(addr)) < 0)
    {
        LOG_ERROR << "::bind error, " << strerror(errno);
        exit(-1);
    }
}

void SsmsSocket::Listen(int fd)
{
    if (::listen(fd, 1024) < 0)
    {
        LOG_ERROR << "::listen error, " << strerror(errno);
        exit(-1);
    }
}

void SsmsSocket::SetAddrReuse(int fd)
{
    int flag = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
    {
        LOG_ERROR << "::setsockopt set addr reuse error, " << strerror(errno);
        exit(-1);
    }
}

void SsmsSocket::SetPortReuse(int fd)
{
    int flag = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)) < 0)
    {
        LOG_ERROR << "::setsockopt set port reuse error, " << strerror(errno);
        exit(-1);
    }
}