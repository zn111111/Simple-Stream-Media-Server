#include "SsmsWebrtcServer.h"
#include "Network/SsmsUdpServer.h"
#include "Base/SsmsLogStream.h"
#include "Network/SsmsSocket.h"
#include "Network/SsmsNetAddress.h"
#include "SsmsWebrtcPlayClient.h"
#include "Live/SsmsLiveManagment.h"
#include "Live/SsmsSession.h"

using namespace ssms::media;

SsmsWebrtcServer::SsmsWebrtcServer(const SsmsLiveManagmentPtr &live_manage,
                                const SsmsNetAddressPtr &server_addr)
: live_manage_(live_manage)
, server_addr_(server_addr)
{

}

void SsmsWebrtcServer::Start()
{
    CheckConnectionStatus();
}

void SsmsWebrtcServer::AddClient(const std::string &four_tuple, const SsmsWebrtcPlayClientPtr &client)
{
    std::lock_guard<std::mutex> lock(lock_);
    if (rtc_clients_.find(four_tuple) == rtc_clients_.end())
    {
        rtc_clients_[four_tuple] = client;
    }
}

void SsmsWebrtcServer::DeleteClient(const std::string &four_tuple)
{
    std::lock_guard<std::mutex> lock(lock_);
    if (rtc_clients_.find(four_tuple) != rtc_clients_.end())
    {
        rtc_clients_.erase(four_tuple);
    }
}

SsmsWebrtcPlayClientPtr SsmsWebrtcServer::GetClient(const std::string &four_tuple)
{
    std::lock_guard<std::mutex> lock(lock_);
    if (rtc_clients_.find(four_tuple) != rtc_clients_.end())
    {
        return rtc_clients_[four_tuple];
    }

    return nullptr;
}

int SsmsWebrtcServer::OnMessage(const SsmsUdpSocketPtr &udp_socket,
                                const SsmsNetAddressPtr &client_addr,
                                const SsmsUdpPktPtr &in_pkt)
{
    UdpPacketType type;
    if (IsStun(in_pkt))
    {
        //通过sdp协议进行媒体和网络协商时没有udp客户端的四元组信息，只能用ufrag把SsmsWebrtcPlayClient存起来
        //客户端第一次发stun bind request时用ufrag把SsmsWebrtcPlayClient取出来，然后删除ufrag和SsmsWebrtcPlayClient的对应关系
        //让插入四元组和SsmsWebrtcPlayClient的对应关系
        SsmsStun stun;
        if (stun.OnMessage(shared_from_this(), udp_socket, client_addr, in_pkt) < 0)
        {
            LOG_ERROR << "process stun message failed";
            return -1;
        }
        return 0;
    }
    else if (IsDtls(in_pkt))
    {
        type = DTLS_PACKET;
    }
    else if (IsRtp(in_pkt))
    {
        printf("===================RTP包\n");
        type = RTP_PACKET;
    }
    else if (IsRtcp(in_pkt))
    {
        printf("===================RTCP包\n");
        type = RTCP_PACKET;
    }
    else
    {
        LOG_ERROR << "unsupported udp packet";
        return -1;
    }

    SsmsWebrtcPlayClientPtr rtc_client;
    rtc_client = GetClient(client_addr->GetFourTuple());
    if (!rtc_client)
    {
        LOG_DEBUG << "client ip: " << client_addr->GetStringIp() << ", port: " << client_addr->GetPort() << ", webrtc client is not exist";
        return -1;
    }
    return rtc_client->OnUdpMessage(udp_socket, type, in_pkt);
}

void SsmsWebrtcServer::CheckConnectionStatus()
{
    uint32_t active_connection_nums = 0;
    std::list<std::string> key_list;
    for (auto it = rtc_clients_.begin(); it != rtc_clients_.end(); ++it)
    {
        if ((it->second)->Alive())
        {
            (it->second)->Reset();
            active_connection_nums++;
        }
        else
        {
            key_list.push_back(it->first);
        }
    }

    for (auto it = key_list.begin(); it != key_list.end(); ++it)
    {
        SsmsSessionPtr sess = live_manage_->GetSession(rtc_clients_[(*it)]->GetSessionName());
        if (sess)
        {
            sess->DeleteConsumer(rtc_clients_[(*it)]);
        }
        LOG_DEBUG << "udp client ip " << rtc_clients_[(*it)]->GetClientAddr()->GetStringIp()
                    << ", port " << rtc_clients_[(*it)]->GetClientAddr()->GetPort()
                    << " status timeout, clear status";
        rtc_clients_.erase(*it);
    }

    LOG_INFO << "Webrtc " << active_connection_nums << " clients active";
}

bool SsmsWebrtcServer::IsStun(const SsmsUdpPktPtr &pkt)
{
    return pkt->len_ >= 20 && pkt->data_[0] >= 0 && pkt->data_[0] <= 3;
}

bool SsmsWebrtcServer::IsDtls(const SsmsUdpPktPtr &pkt)
{
    return pkt->len_ >= 13 && pkt->data_[0] >= 20 && pkt->data_[0] <= 63;
}

bool SsmsWebrtcServer::IsRtp(const SsmsUdpPktPtr &pkt)
{
    return pkt->len_ >= 12 && static_cast<uint8_t>(pkt->data_[0]) >= 128 && static_cast<uint8_t>(pkt->data_[0]) <= 191
            && (static_cast<uint8_t>(pkt->data_[1]) < 192 || static_cast<uint8_t>(pkt->data_[1]) > 223);
}

bool SsmsWebrtcServer::IsRtcp(const SsmsUdpPktPtr &pkt)
{
    return pkt->len_ >= 12 && static_cast<uint8_t>(pkt->data_[0]) >= 128 && static_cast<uint8_t>(pkt->data_[0]) <= 191
            && static_cast<uint8_t>(pkt->data_[1]) >= 192 && static_cast<uint8_t>(pkt->data_[1]) <= 223;
}