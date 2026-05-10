#pragma once

#include <functional>
#include <memory>
#include <arpa/inet.h>
#include "Live/SsmsLiveDefine.h"
#include "Media/SsmsMediaDefine.h"

using namespace ssms::live;
using namespace ssms::media;

namespace ssms
{
    namespace nw
    {
        class SsmsNetAddress;
        using SsmsNetAddressPtr = std::shared_ptr<SsmsNetAddress>;
        
        class SsmsAcceptor;
        using SsmsAcceptorPtr = std::shared_ptr<SsmsAcceptor>;

        class SsmsEvent;
        using SsmsEventPtr = std::shared_ptr<SsmsEvent>;

        class SsmsEventLoopThread;
        using SsmsEventLoopThreadPtr = std::shared_ptr<SsmsEventLoopThread>;

        class TcpConnection;
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

        class SsmsTcpServer;
        using SsmsTcpServerPtr = std::shared_ptr<SsmsTcpServer>;

        class SsmsEventLoopThreadPool;
        using SsmsEventLoopThreadPoolPtr = std::unique_ptr<SsmsEventLoopThreadPool>;

        class SsmsUdpServer;
        using SsmsUdpServerPtr = std::shared_ptr<SsmsUdpServer>;

        class SsmsUdpClient;
        using SsmsUdpClientPtr = std::shared_ptr<SsmsUdpClient>;

        struct SsmsUdpPkt;
        using SsmsUdpPktPtr = std::shared_ptr<struct SsmsUdpPkt>;

        class SsmsUdpSocket;
        using SsmsUdpSocketPtr = std::shared_ptr<SsmsUdpSocket>;

        class SsmsEventLoop;
        using AcceptCallback = std::function<void (SsmsEventLoop *loop, int fd, const SsmsNetAddressPtr &client, const SsmsNetAddressPtr &server, SsmsServerProtocol protocol)>;

        using TaskCallback = std::function<void ()>;

        using BusinessCloseCallback = std::function<void (const TcpConnectionPtr &)>;

        using BusinessReceiveCallback = std::function<void (const TcpConnectionPtr &)>;

        using BusinessWriteCompleteCallback = std::function<void (const SsmsContextPtr &)>;

        using UdpWriteCompleteCallback = std::function<void ()>;

        using UdpCloseCallback = std::function<void ()>;

        using UdpMessageCallback = std::function<int (const SsmsUdpSocketPtr &, const SsmsNetAddressPtr &, const SsmsUdpPktPtr &)>;
    }
}