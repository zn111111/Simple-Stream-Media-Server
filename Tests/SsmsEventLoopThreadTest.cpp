#include "Network/SsmsEventLoopThread.h"
#include "SsmsEventLoopThreadTest.h"
#include "Network/SsmsEventLoop.h"
#include "Network/SsmsEvent.h"

using namespace ssms::nw;

void EventLoopThreadTest()
{
    SsmsEventLoopThread thread;
    thread.Run();
    SsmsEventLoop *loop = thread.Loop();
    SsmsEventPtr event = std::make_shared<SsmsEvent>(0);
    loop->AddEvent(event);
}