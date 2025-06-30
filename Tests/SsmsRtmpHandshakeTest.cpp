#include <memory>
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
#include "Network/SsmsEventLoopThreadPool.h"

using namespace ssms::nw;
using namespace ssms::media;

void RtmpHandshakeTest()
{
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThreadPoolPtr pool = std::make_unique<SsmsEventLoopThreadPool>();
    pool->Start(addr, manage);

    while (true);
}