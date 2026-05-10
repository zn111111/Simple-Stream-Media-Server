#pragma once

#include <string>
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsStun
        {
        public:
            SsmsStun() = default;
            ~SsmsStun() = default;

            int OnMessage(const SsmsWebrtcServerPtr &rtc_server, const SsmsUdpSocketPtr &udp_socket, const SsmsNetAddressPtr &client_addr, const SsmsUdpPktPtr &in_pkt);
            std::string GetLocalUfrag() const;
            std::string GetRemoteUfrag() const;
        private:
            //返回值小于0表示解析失败, 等于0表示已经响应过了, 等于1表示解析成功 
            int Decode(SsmsWebrtcServerPtr rtc_server, const SsmsNetAddressPtr &client_addr, const char *data, int len, std::string &password);
            int Encode(const std::string &password, const SsmsNetAddressPtr &client_addr, SsmsUdpPktPtr &pkt);
            void CalHmac(const std::string &password, const char *src_data, int src_len, char *dst_data);

            //服务端发给客户端的ufrag
            std::string local_ufrag_;
            std::string remote_ufrag_;
            std::string transaction_id_;
        };
    }
}