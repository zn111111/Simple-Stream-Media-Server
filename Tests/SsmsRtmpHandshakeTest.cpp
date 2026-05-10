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
#include "Media/SsmsWebrtcServer.h"

using namespace ssms::nw;
using namespace ssms::media;

void RtmpHandshakeTest()
{
    std::unordered_map<ssms::live::SsmsServerProtocol, ssms::nw::SsmsNetAddressPtr> local_addr_map;
    SsmsNetAddressPtr rtmp_addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 1935, true);
    SsmsNetAddressPtr http_addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 8081, true);
    SsmsNetAddressPtr udp_addr = std::make_shared<SsmsNetAddress>("0.0.0.0", 60000, true);
    local_addr_map.insert(std::make_pair(SsmsServerProtocolRTMP, rtmp_addr));
    local_addr_map.insert(std::make_pair(SsmsServerProtocolHTTP, http_addr));
    SsmsLiveManagmentPtr manage = std::make_shared<SsmsLiveManagment>();
    SsmsEventLoopThreadPoolPtr pool = std::make_unique<SsmsEventLoopThreadPool>();
    SsmsWebrtcServerPtr rtc_server = std::make_shared<SsmsWebrtcServer>(manage, udp_addr);
    pool->Start(local_addr_map, udp_addr, manage, rtc_server);

    while (true);
}