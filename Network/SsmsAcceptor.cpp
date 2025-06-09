#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "SsmsAcceptor.h"
#include "SsmsSocket.h"
#include "Base/SsmsLogStream.h"
#include "SsmsEventLoop.h"
#include "SsmsNetAddress.h"

using namespace ssms::nw;

SsmsAcceptor::SsmsAcceptor(SsmsEventLoop *loop, const SsmsNetAddressPtr &addr, SsmsServerProtocol protocol)
: SsmsEvent(-1)
, loop_(loop)
, addr_(addr)
, protocol_(protocol)
{

}

SsmsAcceptor::~SsmsAcceptor()
{
    loop_->DeleteEvent(fd_);
}

void SsmsAcceptor::SetAcceptCallback(const AcceptCallback &cb)
{
    cb_ = cb;
}

void SsmsAcceptor::SetAcceptCallback(AcceptCallback &&cb)
{
    cb_ = std::move(cb);
}

void SsmsAcceptor::StartListen()
{
    if (fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
    if ((fd_ = SsmsSocket::CreateTcpNonBlockSocket()) < 0)
    {
        LOG_ERROR << "create socket error";
        exit(-1);
    }
    SsmsSocket::SetAddrReuse(fd_);
    SsmsSocket::SetPortReuse(fd_);

    SsmsEventPtr event = std::dynamic_pointer_cast<SsmsAcceptor>(shared_from_this());
    loop_->AddEvent(event);
    struct sockaddr addr;
    addr_->GetSockAddr(&addr);
    SsmsSocket::Bind(fd_, addr);
    SsmsSocket::Listen(fd_);
}

void SsmsAcceptor::OnRead()
{
    OnAccept();
}

void SsmsAcceptor::OnAccept()
{
    struct sockaddr_in6 addr;
    socklen_t len = sizeof(struct sockaddr_in6);
    //一次触发, 循环监听, 直到没有连接事件为止
    while (true)
    {
        memset(&addr, 0, len);
        int fd = ::accept4(fd_, (struct sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        int flag = 1;
        ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        if (fd >= 0)
        {
            SsmsNetAddressPtr netaddr = std::make_shared<SsmsNetAddress>();
            if (AF_INET == addr.sin6_family)
            {
                char ip[16] = {'\0'};
                ::inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr.s_addr, ip, sizeof(ip) - 1);
                netaddr->SetIp(ip, ::ntohs(((struct sockaddr_in *)&addr)->sin_port), true);
            }
            else if (AF_INET6 == addr.sin6_family)
            {
                char ip[40] = {'\0'};
                ::inet_ntop(AF_INET, ((struct sockaddr_in6 *)&addr)->sin6_addr.s6_addr, ip, sizeof(ip) - 1);
                netaddr->SetIp(ip, ::ntohs(((struct sockaddr_in6 *)&addr)->sin6_port), true);
            }
            LOG_DEBUG << "accept connection, fd = " << fd << ", client ip = " << netaddr->GetStringIp() << ", client port = " << netaddr->GetPort();
            if (cb_)
            {
                cb_(loop_, fd, netaddr, addr_, protocol_);
            }
        }
        else
        {
            if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
            {
                LOG_ERROR << "accept connection error";
                OnClose();
            }
            break;
        }
    }
}

//重新打开
void SsmsAcceptor::OnClose()
{
    loop_->DeleteEvent(fd_);
    if (fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
    StartListen();
}

void SsmsAcceptor::OnError()
{
    LOG_WARN << "listen ip " << addr_->GetStringIp() << ", port " << addr_->GetPort() << " error";
    OnClose();
}