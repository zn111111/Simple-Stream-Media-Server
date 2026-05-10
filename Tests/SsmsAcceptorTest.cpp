#include "SsmsAcceptorTest.h"
#include "Network/SsmsAcceptor.h"
#include "Network/SsmsEventLoopThread.h"
#include "Network/SsmsEventLoop.h"
#include "Live/SsmsLiveManagment.h"
#include "Network/SsmsNetAddress.h"
#include "Media/SsmsWebrtcServer.h"

using namespace ssms::nw;
using namespace ssms::live;

void TestAcceptor()
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

    loop.Loop()->AddTask([&acceptor] () {
        acceptor->StartListen();
    });
    while (true);
}
