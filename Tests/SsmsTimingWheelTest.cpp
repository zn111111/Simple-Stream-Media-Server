#include <memory>
#include "SsmsTimingWheelTest.h"
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
#include "Base/SsmsLogStream.h"

void TestTimingWheel()
{
    SsmsEventLoop loop;
    loop.RunEvery(60, [] () {
        LOG_INFO << "1111111111";
    });
    loop.OnWork();
}