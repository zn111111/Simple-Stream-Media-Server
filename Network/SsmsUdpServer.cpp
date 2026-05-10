#include "SsmsUdpServer.h"
#include "SsmsSocket.h"
#include "SsmsNetAddress.h"
#include "SsmsEventLoop.h"
#include "SsmsUdpSocket.h"
#include "SsmsLogStream.h"
#include "Media/SsmsWebrtcServer.h"

using namespace ssms::nw;

SsmsUdpServer::SsmsUdpServer(SsmsEventLoop *loop,
                            const SsmsLiveManagmentPtr &live_manage,
                            SsmsNetAddressPtr server_addr,
                            SsmsWebrtcServerPtr rtc_server)
: SsmsUdpSocket(loop, -1, server_addr, std::make_shared<SsmsNetAddress>())
, loop_(loop)
, live_manage_(live_manage)
, rtc_server_(rtc_server)
{
    SsmsUdpSocket::SetCloseCallback([this] () {
        Start();
    });
}

void SsmsUdpServer::Start()
{
    fd_ = SsmsSocket::CreateUdpNonBlockSocket();
    SsmsSocket::SetAddrReuse(fd_);
    SsmsSocket::SetPortReuse(fd_);
    loop_->AddTask([this] () {
        SsmsEventPtr event = std::dynamic_pointer_cast<SsmsUdpServer>(shared_from_this());
        loop_->AddEvent(event);
        struct sockaddr addr;
        server_addr_->GetSockAddr(&addr);
        SsmsSocket::Bind(fd_, addr);
    });
}

void SsmsUdpServer::SetMessageCallback(const UdpMessageCallback &callback)
{
    SsmsUdpSocket::SetMessageCallback(callback);
}

void SsmsUdpServer::SetMessageCallback(UdpMessageCallback &&callback)
{
    SsmsUdpSocket::SetMessageCallback(std::move(callback));
}

void SsmsUdpServer::SetWriteCompleteCallback(const UdpWriteCompleteCallback &callback)
{
    SsmsUdpSocket::SetWriteCompleteCallback(callback);
}

void SsmsUdpServer::SetWriteCompleteCallback(UdpWriteCompleteCallback &&callback)
{
    SsmsUdpSocket::SetWriteCompleteCallback(std::move(callback));
}

void SsmsUdpServer::OnRead()
{
    SsmsUdpSocket::OnRead();
}

void SsmsUdpServer::OnWrite()
{
    SsmsUdpSocket::OnWrite();
}

void SsmsUdpServer::OnClose()
{
    SsmsUdpSocket::OnClose();

}

void SsmsUdpServer::OnError()
{
    SsmsUdpSocket::OnError();
}