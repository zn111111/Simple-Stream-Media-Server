#pragma once

#include "Network/SsmsNetworkDefine.h"
#include "SsmsDtls.h"
#include "SsmsSdp.h"

using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsWebrtcHandler
        {
            friend class SsmsWebrtcPlayClient;
        public:
            SsmsWebrtcHandler(const SsmsNetAddressPtr &server_addr,
                            SsmsWebrtcServerPtr rtc_server);
            ~SsmsWebrtcHandler() = default;

            int Init();
            int OnMessage(const SsmsUdpSocketPtr &udp_socket, UdpPacketType type, const SsmsUdpPktPtr &pkt, const SsmsNetAddressPtr &client_addr);
            int ProcessSdp(const std::string &method, const char *out_payload, int out_payload_len, SsmsPacketPtr &pkt);
            std::string GetSessionName();
            void SetWebrtcPlayClient(SsmsWebrtcPlayClientPtr rtc_client);
        private:
            std::string app_name_;
            std::string stream_name_;
            SsmsDtls dtls_;
            SsmsSdp sdp_;
            SsmsWebrtcServerPtr rtc_server_;
            SsmsWebrtcPlayClientPtr rtc_client_;
        };
    }
}