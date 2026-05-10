#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include "SsmsMediaDefine.h"
#include "Base/NonCopyable.h"
#include "Network/SsmsEventLoop.h"
#include "SsmsStun.h"

using namespace ssms::base;
using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsWebrtcServer : public NonCopyable, public std::enable_shared_from_this<SsmsWebrtcServer>
        {
            friend class SsmsUdpServer;
        public:
            SsmsWebrtcServer(const SsmsLiveManagmentPtr &live_manage,
                            const SsmsNetAddressPtr &server_addr);
            ~SsmsWebrtcServer() = default;

            void Start();
            //four_tuple是客户端的四元组: 字符串ip + 字符串端口
            void AddClient(const std::string &four_tuple, const SsmsWebrtcPlayClientPtr &client);
            void DeleteClient(const std::string &four_tuple);
            SsmsWebrtcPlayClientPtr GetClient(const std::string &four_tuple);
            int OnMessage(const SsmsUdpSocketPtr &udp_socket,
                        const SsmsNetAddressPtr &client_addr,
                        const SsmsUdpPktPtr &in_pkt);
        private:
            void CheckConnectionStatus();
            bool IsStun(const SsmsUdpPktPtr &pkt);
            bool IsDtls(const SsmsUdpPktPtr &pkt);
            bool IsRtp(const SsmsUdpPktPtr &pkt);
            bool IsRtcp(const SsmsUdpPktPtr &pkt);

            SsmsLiveManagmentPtr live_manage_;
            SsmsNetAddressPtr server_addr_;
            std::mutex lock_;
            std::unordered_map<std::string, SsmsWebrtcPlayClientPtr> rtc_clients_;
        };
    }
}