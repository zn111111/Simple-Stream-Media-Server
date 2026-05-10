#include "SsmsTcpServerTest.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsNetAddress.h"
#include "Network/SsmsTcpServer.h"
#include "Live/SsmsLiveManagment.h"
#include "Media/SsmsWebrtcServer.h"

using namespace ssms::nw;

void TestTcpServer()
{
    std::unordered_map<ssms::live::SsmsServerProtocol, ssms::nw::SsmsNetAddressPtr> local_addr_map;
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsNetAddressPtr udp_addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 60000, true);
    local_addr_map.insert(std::make_pair(SsmsServerProtocolRTMP, addr));
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread loop;
    SsmsWebrtcServerPtr rtc_server = std::make_shared<SsmsWebrtcServer>(manage, udp_addr);
    loop.Run(local_addr_map, udp_addr, manage, rtc_server);
    SsmsTcpServer tcpserver(loop.Loop(), local_addr_map, manage);
    tcpserver.SetReceiveCallback([] (const TcpConnectionPtr &conn) {
        char buffer[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!\r\n";
        conn->SendPkt(buffer, strlen(buffer));
    });
    tcpserver.Start(rtc_server);
    while (true);
}