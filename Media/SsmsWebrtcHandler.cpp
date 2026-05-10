#include "SsmsWebrtcHandler.h"
#include "Base/SsmsLogStream.h"
#include "nlohmann/json.hpp"
#include "SsmsPacket.h"
#include "SsmsUtils.h"
#include "Network/SsmsNetAddress.h"
#include "SsmsWebrtcServer.h"
#include "Network/SsmsUdpPkt.h"

using namespace ssms::media;

SsmsWebrtcHandler::SsmsWebrtcHandler(const SsmsNetAddressPtr &server_addr,
                                    SsmsWebrtcServerPtr rtc_server)
: sdp_(server_addr->GetStringIp(), std::to_string(server_addr->GetPort()))
, rtc_server_(rtc_server)
{

}

int SsmsWebrtcHandler::Init()
{
    if (!dtls_.Init())
    {
        LOG_ERROR << "SsmsDtls init failed";
        return -1;
    }
    sdp_.SetFingerprint(dtls_.Fingerprint());

    return 0;
}

int SsmsWebrtcHandler::OnMessage(const SsmsUdpSocketPtr &udp_socket, UdpPacketType type, const SsmsUdpPktPtr &pkt, const SsmsNetAddressPtr &client_addr)
{
    switch (type)
    {
        case DTLS_PACKET:
            dtls_.OnRecv(udp_socket, pkt);
            break;
        case RTP_PACKET:
            break;
        case RTCP_PACKET:
            break;
        default:
            LOG_ERROR << "unsupported udp packet";
            return -1;
    }

    return 0;
}

int SsmsWebrtcHandler::ProcessSdp(const std::string &method, const char *out_payload, int out_payload_len, SsmsPacketPtr &pkt)
{
    std::stringstream stream;
    auto parsed = nlohmann::json::parse(out_payload, out_payload + out_payload_len);
    if (!parsed.contains("sdp"))
    {
        LOG_ERROR << "request format error, no sdp";
        return -1;
    }
    std::string sdp = parsed["sdp"];
    if (parsed.contains("streamurl"))
    {
        std::string url = parsed["streamurl"];
        std::vector<std::string> v = SsmsUtils::Split(url, "/");
        if (v.size() < 2)
        {
            LOG_ERROR << "request format error, no complete stream url";
            return -1;
        }
        app_name_ = v[v.size() - 2];
        stream_name_ = v[v.size() - 1];
        sdp_.SetStream(stream_name_);
    }
    if (sdp_.Decode(sdp) < 0)
    {
        LOG_ERROR << "decode sdp failed";
        return -1;
    }

    nlohmann::json json;
    json["code"] = 0;
    json["server"] = "ssms";
    json["sdp"] = sdp_.Encode();
    json["sessionid"] = sdp_.GetRemoteUfrag() + sdp_.GetLocalUfrag();
    std::string dumped_json = json.dump(4);

    stream << "HTTP/1.1 200 OK\r\n"
    << "Server: ssms\r\n"
    << "Content-length: " << dumped_json.size() << "\r\n"
    << "Content-type: text/plain\r\n"
    << "Access-Control-Allow-Origin: *\r\n"
    << "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
    << "Allow: POST, GET, OPTIONS*\r\n"
    << "Access-Control-Allow-Headers: Content-type\r\n"
    << "Connection: close\r\n\r\n"
    << std::move(dumped_json);
    pkt = std::make_shared<SsmsPacket>(stream.str().size());
    memcpy(pkt->data, stream.str().c_str(), stream.str().size());
    rtc_server_->AddClient(sdp_.GetLocalUfrag(), rtc_client_);

    return 0;
}

std::string SsmsWebrtcHandler::GetSessionName()
{
    return app_name_ + "/" + stream_name_;
}

void SsmsWebrtcHandler::SetWebrtcPlayClient(SsmsWebrtcPlayClientPtr rtc_client)
{
    rtc_client_ = rtc_client;
}