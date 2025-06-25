#include "SsmsTcpServer.h"
#include "SsmsTcpConnection.h"
#include "SsmsEventLoop.h"
#include "Media/SsmsRtmpMessageContext.h"

using namespace ssms::nw;

SsmsTcpServer::SsmsTcpServer(SsmsEventLoop *loop, const SsmsNetAddressPtr &local_addr, SsmsLiveManagmentPtr &live_manage)
: loop_(loop)
, rtmp_acceptor_(std::make_shared<SsmsAcceptor>(loop, local_addr, SsmsServerProtocolRTMP))
, live_manage_(live_manage)
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