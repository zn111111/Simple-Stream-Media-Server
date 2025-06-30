#include "SsmsThreadLoopReadWriteTest.h"
#include "Network/SsmsEventLoop.h"
#include "Network/SsmsAcceptor.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsTcpConnection.h"
#include "Base/SsmsLogStream.h"
#include "Network/SsmsNetAddress.h"
#include "Live/SsmsLiveManagment.h"

using namespace ssms::nw;
using namespace ssms::live;

HttpContextTest::HttpContextTest(const TcpConnectionPtr &conn)
: conn_(conn)
{

}

int HttpContextTest::Parse(const SsmsBufferPtr &data)
{
    char ret[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!\r\n";
    conn_.lock()->SendPktInLoop(ret, strlen(ret));
    return 0;
}

void TestRead()
{
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread loop;
    loop.Run(addr, manage);
    SsmsAcceptorPtr acceptor = std::make_shared<SsmsAcceptor>(loop.Loop(), addr, SsmsServerProtocolRTMP);
    acceptor->SetAcceptCallback([] (SsmsEventLoop *loop, int fd, const SsmsNetAddressPtr &local, const SsmsNetAddressPtr &remote, SsmsServerProtocol protocol) {
        TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop, local, remote, fd);
        SsmsContextPtr context = std::make_shared<HttpContextTest>(conn);
        conn->SetContext(context);
        loop->AddTask([loop, conn] () {
            loop->AddEvent(conn);
        });
        LOG_TRACE << "do callback";
    });

    loop.Loop()->AddTask([&acceptor] () {
        acceptor->StartListen();
    });    

    while (true);
}