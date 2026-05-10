#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include "SsmsTcpConnection.h"
#include "SsmsEventLoop.h"
#include "Base/SsmsLogStream.h"
#include "Media/SsmsContext.h"
#include "SsmsNetAddress.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Media/SsmsPlayClient.h"
#include "Live/SsmsSession.h"
#include "Live/SsmsLiveManagment.h"
#include "Media/SsmsPublishClient.h"

using namespace ssms::nw;
using namespace ssms::media;

TcpConnection::TcpConnection(SsmsEventLoop *loop, const SsmsNetAddressPtr &client_addr, SsmsNetAddressPtr server_addr, int fd)
: SsmsConnection(loop, client_addr, server_addr, fd)
, buffer_(std::make_shared<SsmsBuffer>())
{

}

TcpConnection::~TcpConnection()
{
    
}

void TcpConnection::SetContext(const SsmsContextPtr &context)
{
    context_ = context;
}

void TcpConnection::SetWriteCompleteCallback(const BusinessWriteCompleteCallback &callback)
{
    write_callback_ = callback;
}

void TcpConnection::SetWriteCompleteCallback(BusinessWriteCompleteCallback &&callback)
{
    write_callback_ = std::move(callback);
}

void TcpConnection::SetCloseCallback(const BusinessCloseCallback &callback)
{
    close_callback_ = callback;
}

void TcpConnection::SetCloseCallback(BusinessCloseCallback &&callback)
{
    close_callback_ = std::move(callback);
}

void TcpConnection::OnRead()
{
    if (closed_)
    {
        LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        return;
    }

    while (true)
    {
        int err = 0;
        int ret = buffer_->readFd(fd_, &err);
        if (ret < 0)
        {
            if (EINTR != err && EAGAIN != err && EWOULDBLOCK != err)
            {
                LOG_ERROR << "read error, " << strerror(errno);
                OnClose();
            }
            break;
        }
        else if (0 == ret)
        {
            LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
            OnClose();
            break;
        }
        else
        {
            connection_alive_ = true;
            int ret = context_->Parse(buffer_);
            if (ret < 0)
            {
                LOG_ERROR << "protocol parse error";
                OnClose();
                break;
            }
            else if (2 == ret)
            {
                OnClose();
                break;
            }
        }
    }
}

void TcpConnection::OnWrite()
{
    if (closed_)
    {
        LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        return;
    }

    while (!iovecs_.empty())
    {
        const struct iovec *p_vec = &iovecs_[0];
        int n_vec = (iovecs_.size() <= IOV_MAX ? iovecs_.size() : IOV_MAX);
        int ret = ::writev(fd_, p_vec, n_vec);
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
            while (ret > 0)
            {
                if (ret < iovecs_[0].iov_len)
                {
                    iovecs_[0].iov_base = (char *)(iovecs_[0].iov_base) + ret;
                    iovecs_[0].iov_len -= ret;
                    break;
                }
                else
                {
                    ret -= iovecs_[0].iov_len;
                    iovecs_.erase(iovecs_.begin());
                }
            }
        }
    }

    if (iovecs_.empty())
    {
        //关闭写
        EnableWriteEvent(false);
        write_callback_(context_);
    }
}

void TcpConnection::OnClose()
{
    loop_->DeleteEvent(fd_);
    SsmsPlayClientPtr player = std::dynamic_pointer_cast<SsmsPlayClient>(context_->client_);
    SsmsPublishClientPtr publisher = std::dynamic_pointer_cast<SsmsPublishClient>(context_->client_);
    if (player && player->sess_)
    {
        player->sess_->DeleteConsumer(player);
    }
    else if (publisher)
    {
        context_->live_manage_->DeleteSession(context_->app_ + "/" + context_->stream_);
    }
    if (close_callback_)
    {
        close_callback_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
    }
    SsmsEvent::OnClose();
}

void TcpConnection::OnError()
{
    int err = 0;
    socklen_t len = sizeof(err);
    ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
    LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", error: " << strerror(err);
    OnClose();
}

void TcpConnection::SendPkt(char *data, uint32_t len)
{
    loop_->AddTask([this, data, len] () {
        OnSendPkt(data, len);
    });
}

void TcpConnection::SendPktInLoop(char *data, uint32_t len)
{
    OnSendPkt(data, len);
}

void TcpConnection::SendNode(const struct iovec &iovec)
{
    if (closed_)
    {
        LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        return;
    }

    int ret = ::writev(fd_, &iovec, 1);
    if (ret < 0)
    {
        if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
        {
            LOG_ERROR << "writev error, " << strerror(errno);
            OnClose();
            return;
        }
    }
    
    connection_alive_ = true;
    int size = iovec.iov_len;
    if (size > ret)
    {
        struct iovec ovec;
        int to_be_send_bytes = size - ret;
        ovec.iov_base = (char *)(iovec.iov_base) + ret;
        ovec.iov_len = to_be_send_bytes;
        iovecs_.emplace_back(ovec);
        //开启写
        EnableWriteEvent(true);
    }
    else
    {
        write_callback_(context_);
    }
}

void TcpConnection::OnSendPkt(char *data, uint32_t len)
{
    if (!data || 0 == len || closed_)
    {
        return;
    }

    int ret = 0;
    if (iovecs_.empty())
    {
        ret = ::write(fd_, data, len);
        if (ret < 0)
        {
            if (EINTR != errno && EAGAIN != errno && EWOULDBLOCK != errno)
            {
                LOG_ERROR << "send packet errror, " << strerror(errno);
                OnClose();
                return;
            }

            ret = 0;
        }
    }

    connection_alive_ = true;
    if (ret < len)
    {
        struct iovec node;
        node.iov_base = data + ret;
        node.iov_len = len - ret;
        iovecs_.emplace_back(node);
        //开启写
        EnableWriteEvent(true);
    }
}

void TcpConnection::SendNodes(const std::list<struct iovec> &iovecs)
{
    if (closed_)
    {
        LOG_WARN << "client ip " << client_addr_->GetStringIp() << ", port " << client_addr_->GetPort() << ", fd " << fd_ << ", closed";
        return;
    }

    struct iovec iovec;
    for (auto it = iovecs.begin(); it != iovecs.end(); ++it)
    {
        iovec.iov_base = (*it).iov_base;
        iovec.iov_len = (*it).iov_len;
        iovecs_.emplace_back(iovec);
    }

    while (!iovecs_.empty())
    {
        const struct iovec *p_vec = &iovecs_[0];
        int n_vec = (iovecs_.size() <= IOV_MAX ? iovecs_.size() : IOV_MAX);
        int ret = ::writev(fd_, p_vec, n_vec);
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
            int i = 0;
            int size = 0;
            for (; i < iovecs_.size(); i++)
            {
                size += iovecs_[i].iov_len;
                if (size < ret)
                {
                    continue;
                }
                else if (size == ret)
                {
                    i++;
                    break;
                }
                else
                {
                    int to_be_send_bytes = size - ret;
                    int sended_bytes = iovecs_[0].iov_len - to_be_send_bytes;
                    iovecs_[i].iov_base = (char *)(iovecs_[0].iov_base) + sended_bytes;
                    iovecs_[i].iov_len = to_be_send_bytes;
                    break;
                }
            }
            iovecs_.erase(iovecs_.begin(), iovecs_.begin() + i);
        }
    }

    if (!iovecs_.empty())
    {
        //开启写
        EnableWriteEvent(true);
    }
    else
    {
        write_callback_(context_);
    }
}