#include "SsmsUdpSocket.h"
#include "SsmsLogStream.h"
#include "SsmsNetAddress.h"
#include "SsmsEventLoop.h"

using namespace ssms::nw;

SsmsUdpSocket::SsmsUdpSocket(SsmsEventLoop *loop, int fd, SsmsNetAddressPtr server_addr, const SsmsNetAddressPtr &client_addr)
: SsmsConnection(loop, client_addr, server_addr, fd)
, buffer_(std::make_shared<SsmsUdpPkt>(65535))
{

}

void SsmsUdpSocket::SetMessageCallback(const UdpMessageCallback &callback)
{
    message_callback_ = callback;
}

void SsmsUdpSocket::SetMessageCallback(UdpMessageCallback &&callback)
{
    message_callback_ = std::move(callback);
}

void SsmsUdpSocket::SetWriteCompleteCallback(const UdpWriteCompleteCallback &callback)
{
    write_callback_ = callback;
}

void SsmsUdpSocket::SetWriteCompleteCallback(UdpWriteCompleteCallback &&callback)
{
    write_callback_ = std::move(callback);
}

void SsmsUdpSocket::SetCloseCallback(const UdpCloseCallback &callback)
{
    close_callback_ = callback;
}

void SsmsUdpSocket::SetCloseCallback(UdpCloseCallback &&callback)
{
    close_callback_ = std::move(callback);
}

void SsmsUdpSocket::OnRead()
{
    if (closed_)
    {
        if (client_addr_)
        {
            LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        }
        else
        {
            LOG_WARN << "client fd " << fd_ << ", closed";
        }
        return;
    }

    while (true)
    {
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        socklen_t addr_len = sizeof(addr);
        int ret = ::recvfrom(fd_, buffer_->data_, buffer_->capacity_, 0, (struct sockaddr *)&addr, &addr_len);
        if (ret < 0)
        {
            if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
            {
                LOG_ERROR << "read error, " << strerror(errno);
                OnClose();
            }
            break;
        }
        else if (ret > 0)
        {
            connection_alive_ = true;

            if (AF_INET == addr.sin6_family)
            {
                char ip[16] = {'\0'};
                ::inet_ntop(AF_INET, &((struct sockaddr_in *)&addr)->sin_addr.s_addr, ip, sizeof(ip) - 1);
                client_addr_->SetIp(ip, ::ntohs(((struct sockaddr_in *)&addr)->sin_port), true);
            }
            else if (AF_INET6 == addr.sin6_family)
            {
                char ip[40] = {'\0'};
                ::inet_ntop(AF_INET, ((struct sockaddr_in6 *)&addr)->sin6_addr.s6_addr, ip, sizeof(ip) - 1);
                client_addr_->SetIp(ip, ::ntohs(((struct sockaddr_in6 *)&addr)->sin6_port), false);
            }

            if (message_callback_)
            {
                buffer_->len_ = ret;
                if (message_callback_(std::dynamic_pointer_cast<SsmsUdpSocket>(shared_from_this()), client_addr_, buffer_) < 0)
                {
                    LOG_DEBUG << "process udp packet failed";
                    continue;
                }
            }
        }
    }
}

void SsmsUdpSocket::OnWrite()
{
    if (closed_)
    {
        LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        return;
    }

    while (!pkt_list_.empty())
    {
        struct sockaddr addr;
        client_addr_->GetSockAddr(&addr);
        int ret = ::sendto(fd_, pkt_list_.front()->data_, pkt_list_.front()->len_, 0, &addr, sizeof(addr));
        if (ret < 0)
        {
            if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
            {
                LOG_ERROR << "writev error, " << strerror(errno);
                OnClose();
                return;
            }
            break;
        }
        else
        {
            connection_alive_ = true;
            pkt_list_.pop_front();
        }
    }

    if (pkt_list_.empty() && write_callback_)
    {
        write_callback_();
    }
}

void SsmsUdpSocket::OnClose()
{
    loop_->DeleteEvent(fd_);
    SsmsEvent::OnClose();
    if (close_callback_)
    {
        close_callback_();
    }
}

void SsmsUdpSocket::OnError()
{
    int err = 0;
    socklen_t len = sizeof(err);
    ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
    LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", error: " << strerror(err);
    OnClose();
}

void SsmsUdpSocket::SendPacket(const SsmsUdpPktPtr &pkt)
{
    if (closed_)
    {
        LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        return;
    }

    struct sockaddr addr;
    client_addr_->GetSockAddr(&addr);
    int ret = ::sendto(fd_, pkt->data_, pkt->len_, 0, &addr, sizeof(addr));
    if (ret < 0)
    {
        if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
        {
            LOG_ERROR << "writev error, " << strerror(errno);
            OnClose();
            return;
        }
        pkt_list_.push_back(pkt);
    }
    else
    {
        connection_alive_ = true;
    }
}