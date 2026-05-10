#include "SsmsThreadLoopReadWriteTest.h"
#include "Network/SsmsEventLoop.h"
#include "Network/SsmsAcceptor.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsTcpConnection.h"
#include "Base/SsmsLogStream.h"
#include "Network/SsmsNetAddress.h"
#include "Live/SsmsLiveManagment.h"
#include "Media/SsmsWebrtcServer.h"

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
    std::unordered_map<ssms::live::SsmsServerProtocol, ssms::nw::SsmsNetAddressPtr> local_addr_map;
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsNetAddressPtr udp_addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 60000, true);
    local_addr_map.insert(std::make_pair(SsmsServerProtocolRTMP, addr));
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread loop;
    SsmsWebrtcServerPtr rtc_server = std::make_shared<SsmsWebrtcServer>(manage, udp_addr);
    loop.Run(local_addr_map, udp_addr, manage, rtc_server);
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