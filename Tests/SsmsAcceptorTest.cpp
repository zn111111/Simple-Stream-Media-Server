#include "SsmsAcceptorTest.h"
#include "Network/SsmsAcceptor.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsEventLoop.h"
#include "Live/SsmsLiveManagment.h"
#include "Network/SsmsNetAddress.h"

using namespace ssms::nw;
using namespace ssms::live;

void TestAcceptor()
{
    SsmsEventLoopThread loop;
    loop.Run();
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsAcceptorPtr acceptor = std::make_shared<SsmsAcceptor>(loop.Loop(), addr, SsmsServerProtocolRTMP);

    loop.Loop()->AddTask([&acceptor] () {
        acceptor->StartListen();
    });
    while (true);
}
