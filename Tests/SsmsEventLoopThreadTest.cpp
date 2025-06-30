#include "Network/SsmsEventLoopThread.h"
#include "SsmsEventLoopThreadTest.h"
#include "Network/SsmsEventLoop.h"
#include "Network/SsmsEvent.h"
#include "Live/SsmsLiveManagment.h"
#include "Network/SsmsNetAddress.h"

using namespace ssms::nw;
using namespace ssms::live;

void EventLoopThreadTest()
{
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread thread;
    thread.Run(addr, manage);
    SsmsEventLoop *loop = thread.Loop();
    SsmsEventPtr event = std::make_shared<SsmsEvent>(0);
    loop->AddEvent(event);
}