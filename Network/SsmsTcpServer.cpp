#include <list>
#include "SsmsTcpServer.h"
#include "SsmsTcpConnection.h"
#include "SsmsEventLoop.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Base/SsmsINIReader.h"
#include "Base/SsmsLogStream.h"
#include "SsmsNetAddress.h"
#include "Media/SsmsHttpMessageContext.h"

using namespace ssms::nw;

SsmsTcpServer::SsmsTcpServer(SsmsEventLoop *loop, const std::unordered_map<SsmsServerProtocol, SsmsNetAddressPtr> &local_addr_map, const SsmsLiveManagmentPtr &live_manage)
: loop_(loop)
, live_manage_(live_manage)
, connection_timeout_(S_SSMSCONFIG->GetInteger("COMMON", "tcp_connection_timeout", 30))
{
    std::unordered_map<SsmsServerProtocol, SsmsNetAddressPtr>::const_iterator iter_rtmp;
    std::unordered_map<SsmsServerProtocol, SsmsNetAddressPtr>::const_iterator iter_http;
    if ((iter_rtmp = local_addr_map.find(SsmsServerProtocolRTMP)) == local_addr_map.end()
        || (iter_http = local_addr_map.find(SsmsServerProtocolHTTP)) == local_addr_map.end())
    {
        return;
    }
    rtmp_acceptor_ = std::make_shared<SsmsAcceptor>(loop, iter_rtmp->second, SsmsServerProtocolRTMP);
    http_acceptor_ = std::make_shared<SsmsAcceptor>(loop, iter_http->second, SsmsServerProtocolHTTP);
}

SsmsTcpServer::~SsmsTcpServer()
{

}

void SsmsTcpServer::SetReceiveCallback(const BusinessReceiveCallback &callback)
{
    recv_callback_ = callback;
}

void SsmsTcpServer::SetReceiveCallback(BusinessReceiveCallback &&callback)
{
    recv_callback_ = std::move(callback);
}

void SsmsTcpServer::SetWriteCompleteCallback(const BusinessWriteCompleteCallback &callback)
{
    write_callback_ = callback;
}

void SsmsTcpServer::SetWriteCompleteCallback(BusinessWriteCompleteCallback &&callback)
{
    write_callback_ = std::move(write_callback_);
}

void SsmsTcpServer::SetCloseCallback(const BusinessCloseCallback &callback)
{
    close_callback_ = callback;
}

void SsmsTcpServer::SetCloseCallback(BusinessCloseCallback &&callback)
{
    close_callback_ = std::move(callback);
}

void SsmsTcpServer::Start(SsmsWebrtcServerPtr rtc_server)
{
    loop_->AddTask([this, rtc_server] () {
        rtmp_acceptor_->SetAcceptCallback([this, rtc_server] (SsmsEventLoop *loop,
                                                int fd,
                                                const SsmsNetAddressPtr &local,
                                                const SsmsNetAddressPtr &remote,
                                                SsmsServerProtocol protocol)
        {
            AfterAccept(loop, fd, local, remote, protocol, rtc_server);
        });
        rtmp_acceptor_->StartListen();

        http_acceptor_->SetAcceptCallback([this, rtc_server] (ssms::nw::SsmsEventLoop *loop,
                                            int fd,
                                            const ssms::nw::SsmsNetAddressPtr &local,
                                            const ssms::nw::SsmsNetAddressPtr &remote,
                                            ssms::live::SsmsServerProtocol protocol)
        {
            AfterAccept(loop, fd, local, remote, protocol, rtc_server);
        });
        http_acceptor_->StartListen();
    });

    loop_->RunEvery(30, [this] () {
        CheckConnectionStatus(loop_);
    });
}

void SsmsTcpServer::AfterAccept(SsmsEventLoop *loop,
                                int fd,
                                const SsmsNetAddressPtr &local,
                                const SsmsNetAddressPtr &remote,
                                SsmsServerProtocol protocol,
                                SsmsWebrtcServerPtr rtc_server)
{
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop, local, remote, fd);
    SsmsContextPtr context;
    switch (protocol)
    {
        case SsmsServerProtocolRTMP:
            context = std::make_shared<SsmsRtmpMessageContext>(loop, conn, live_manage_);
            break;
        case SsmsServerProtocolHTTP:
        {
            context = std::make_shared<SsmsHttpMessageContext>(loop, conn, live_manage_, rtc_server);
        }
            break;
        default:
            exit(-1);
    }
    conn->SetContext(context);
    conn->SetCloseCallback([this] (const TcpConnectionPtr &tcp_conn) {
        AfterClose(tcp_conn);
    });
    conn->SetWriteCompleteCallback([this] (const SsmsContextPtr &conn_context) {
        conn_context->ClearSendCompleteData();
        if (write_callback_)
        {
            write_callback_(conn_context);
        }
    });
    loop_->AddEvent(conn);
    connections_.insert(conn);
}

void SsmsTcpServer::AfterClose(const TcpConnectionPtr &conn)
{
    connections_.erase(conn);
    if (close_callback_)
    {
        close_callback_(conn);
    }
}

void SsmsTcpServer::CheckConnectionStatus(SsmsEventLoop *loop)
{
    uint32_t active_connection_nums = 0;
    std::list<TcpConnectionPtr> conn_list;
    for (auto it = connections_.begin(); it != connections_.end(); ++it)
    {
        if ((*it)->Alive())
        {
            (*it)->Reset();
            active_connection_nums++;
        }
        else
        {
            conn_list.push_back(*it);
        }
    }

    for (auto it = conn_list.begin(); it != conn_list.end();)
    {
        connections_.erase(*it);
        LOG_DEBUG << "client ip " << (*it)->client_addr_->GetStringIp() << " fd " << (*it)->Fd() << " connection timeout, close the connection";
        (*it)->OnClose();
        it = conn_list.erase(it);
    }

    LOG_INFO << "thread id " << std::this_thread::get_id() << ", " << active_connection_nums << " connections active";
}