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
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread loop;
    loop.Run(addr, manage);
    SsmsAcceptorPtr acceptor = std::make_shared<SsmsAcceptor>(loop.Loop(), addr, SsmsServerProtocolRTMP);

    loop.Loop()->AddTask([&acceptor] () {
        acceptor->StartListen();
    });
    while (true);
}
