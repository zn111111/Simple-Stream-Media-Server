#include <list>
#include "SsmsTcpServer.h"
#include "SsmsTcpConnection.h"
#include "SsmsEventLoop.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Base/SsmsINIReader.h"
#include "Base/SsmsLogStream.h"
#include "SsmsNetAddress.h"

using namespace ssms::nw;

SsmsTcpServer::SsmsTcpServer(SsmsEventLoop *loop, const SsmsNetAddressPtr &local_addr, const SsmsLiveManagmentPtr &live_manage)
: loop_(loop)
, rtmp_acceptor_(std::make_shared<SsmsAcceptor>(loop, local_addr, SsmsServerProtocolRTMP))
, live_manage_(live_manage)
, connection_timeout_(S_SSMSCONFIG->GetInteger("COMMON", "tcp_connection_timeout", 30))
{

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

void SsmsTcpServer::Start()
{
    loop_->AddTask([this] () {
        rtmp_acceptor_->SetAcceptCallback([this] (SsmsEventLoop *loop,
                                                int fd,
                                                const SsmsNetAddressPtr &local,
                                                const SsmsNetAddressPtr &remote,
                                                SsmsServerProtocol protocol)
        {
            AfterAccept(loop, fd, local, remote, protocol);
        });
        rtmp_acceptor_->StartListen();
    });

    loop_->RunEvery(30, [this] () {
        CheckConnectionStatus(loop_);
    });
}

void SsmsTcpServer::AfterAccept(SsmsEventLoop *loop, int fd, const SsmsNetAddressPtr &local, const SsmsNetAddressPtr &remote, SsmsServerProtocol protocol)
{
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop, local, remote, fd);
    SsmsContextPtr context;
    switch (protocol)
    {
        case SsmsServerProtocolRTMP:
            context = std::make_shared<SsmsRtmpMessageContext>(loop, conn, live_manage_);
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

    LOG_INFO << "thread id " << std::this_thread::get_id() << ", " << active_connection_nums << "connections active";
}