#include "SsmsTcpServerTest.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsNetAddress.h"
#include "Network/SsmsTcpServer.h"
#include "Live/SsmsLiveManagment.h"

using namespace ssms::nw;

void TestTcpServer()
{
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread loop;
    loop.Run(addr, manage);
    SsmsTcpServer tcpserver(loop.Loop(), addr, manage);
    tcpserver.SetReceiveCallback([] (const TcpConnectionPtr &conn) {
        char buffer[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!\r\n";
        conn->SendPkt(buffer, strlen(buffer));
    });
    tcpserver.Start();
    while (true);
}