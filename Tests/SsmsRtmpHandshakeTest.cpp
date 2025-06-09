#include "SsmsRtmpHandshakeTest.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsNetAddress.h"
#include "Network/SsmsAcceptor.h"
#include "Network/SsmsTcpConnection.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Network/SsmsEventLoop.h"
#include "Network/SsmsTcpConnection.h"
#include "Live/SsmsLiveManagment.h"
#include "Network/SsmsTcpServer.h"

using namespace ssms::nw;
using namespace ssms::media;

void RtmpHandshakeTest()
{
    SsmsEventLoopThread loop;
    loop.Run();
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsTcpServer tcpserver(loop.Loop(), addr, manage);
    tcpserver.Start();

    while (true);
}