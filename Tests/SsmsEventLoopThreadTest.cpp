#include "Network/SsmsEventLoopThread.h"
#include "SsmsEventLoopThreadTest.h"
#include "Network/SsmsEventLoop.h"
#include "Network/SsmsEvent.h"
#include "Live/SsmsLiveManagment.h"
#include "Network/SsmsNetAddress.h"
#include "Media/SsmsWebrtcServer.h"

using namespace ssms::nw;
using namespace ssms::live;

void EventLoopThreadTest()
{
    std::unordered_map<ssms::live::SsmsServerProtocol, ssms::nw::SsmsNetAddressPtr> local_addr_map;
    SsmsNetAddressPtr addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsNetAddressPtr udp_addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 60000, true);
    local_addr_map.insert(std::make_pair(SsmsServerProtocolRTMP, addr));
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThread thread;
    SsmsWebrtcServerPtr rtc_server = std::make_shared<SsmsWebrtcServer>(manage, udp_addr);
    thread.Run(local_addr_map, udp_addr, manage, rtc_server);
    SsmsEventLoop *loop = thread.Loop();
    SsmsEventPtr event = std::make_shared<SsmsEvent>(0);
    loop->AddEvent(event);
}