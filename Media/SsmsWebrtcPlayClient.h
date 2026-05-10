#pragma once

#include "SsmsClient.h"
#include "Base/SsmsBuffer.h"
#include "SsmsDtlsCerts.h"
#include "SsmsSdp.h"
#include "SsmsWebrtcHandler.h"

using namespace ssms::base;

namespace ssms
{
    namespace media
    {
        class SsmsWebrtcPlayClient : public std::enable_shared_from_this<SsmsWebrtcPlayClient>
        {
        public:
            SsmsWebrtcPlayClient(const SsmsLiveManagmentPtr &live_manage,
                                SsmsEventLoop *loop,
                                const SsmsNetAddressPtr &server_addr,
                                SsmsWebrtcServerPtr rtc_server);
            ~SsmsWebrtcPlayClient() = default;

            int Init();
            void Active();
            bool Alive() const;
            void Reset();
            std::string GetSessionName();
            std::string GetLocalPassword();
            void SetClientAddr(const SsmsNetAddressPtr &client_addr);
            SsmsNetAddressPtr GetClientAddr() const;
            int ProcessSdp(const std::string &method, const char *out_payload, int out_payload_len, SsmsPacketPtr &pkt);
            int OnUdpMessage(const SsmsUdpSocketPtr &udp_socket, UdpPacketType type, const SsmsUdpPktPtr &pkt);
        private:
            SsmsWebrtcHandler handler_;
            //连接是否活跃, 有数据交换表示活跃
            bool connection_alive_{false};
            SsmsNetAddressPtr client_addr_;
        };
    }
}