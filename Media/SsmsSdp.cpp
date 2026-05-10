#include <sstream>
#include <random>
#include "SsmsSdp.h"
#include "Base/SsmsUtils.h"

using namespace ssms::media;
using namespace ssms::base;

static std::string username_pwd_table = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static std::string rtpmap_flag = "a=rtpmap:";
static std::string ufrag_flag = "a=ice-ufrag:";
static std::string pwd_flag = "a=ice-pwd:";

SsmsSdp::SsmsSdp(std::string server_addr, std::string server_port)
: server_addr_(server_addr)
, server_port_(server_port)
{
    
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<> rand1(0, username_pwd_table.size() - 1);
    for (int i = 0; i < 8; i++)
    {
        local_ufrag_.push_back(username_pwd_table[rand1(mt)]);
    }
    for (int i = 0; i < 32; i++)
    {
        local_pwd_.push_back(username_pwd_table[rand1(mt)]);
    }

    std::uniform_int_distribution<> rand2(10000000, 99999999);
    audio_ssrc_ = rand2(mt);
    video_ssrc_ = audio_ssrc_ + 1;
}

int SsmsSdp::Decode(const std::string &sdp)
{
    int ret = -1;
    std::vector<std::string> v1 = SsmsUtils::Split(sdp, "\r\n");
    for (int i = 0; i < v1.size(); i++)
    {
        int pos = 0;
        if ((pos = v1[i].find(rtpmap_flag)) != std::string::npos)
        {
            int end = 0;
            if ((end = v1[i].find(" ", pos + rtpmap_flag.size())) != std::string::npos)
            {
                int type = atoi(v1[i].substr(pos + rtpmap_flag.size(), end - pos - rtpmap_flag.size()).c_str());
                std::vector<std::string> v2 = SsmsUtils::Split(v1[i].substr(end + 1), "/");
                if (v2.size() >= 2 && v2[0] == "H264")
                {
                    video_payload_type_ = type;
                }
                else if (v2.size() >= 2 && v2[0] == "opus")
                {
                    audio_payload_type_ = type;
                }
            }
        }
        else if ((pos = v1[i].find(ufrag_flag)) != std::string::npos)
        {
            remote_ufrag_ = v1[i].substr(pos + ufrag_flag.size());
        }
        else if ((pos = v1[i].find(pwd_flag)) != std::string::npos)
        {
            remote_pwd_ = v1[i].substr(pos + pwd_flag.size());
        }

        if (-1 != video_payload_type_ && -1 != audio_payload_type_ && !remote_ufrag_.empty() && !remote_pwd_.empty())
        {
            ret = 0;
            break;
        }
    }

    return ret;
}

std::string SsmsSdp::Encode()
{
    std::ostringstream oss;
    oss << "v=0\r\n"
    << "o=ssms 792876392098127638 2 IN IP4 0.0.0.0\r\n"
    << "s=" << stream_name_ << "\r\n"
    << "c=IN IP4 0.0.0.0\r\n"
    //必须位于媒体描述前面, 且在v=、o=、s=和c=之后
    << "t=0 0\r\n"
    << "a=group:BUNDLE 0 1\r\n"
    << "a=msid-semantic: WMS " << stream_name_ << "\r\n";
    if (-1 != audio_payload_type_)
    {
        oss << "m=audio 9 UDP/TLS/RTP/SAVPF " << audio_payload_type_ << "\r\n"
        << "a=rtpmap:" << audio_payload_type_ << " opus/48000/2\r\n"
        << "a=fmtp:" << audio_payload_type_ << " minptime=10;useinbandfec=1\r\n"
        // << "c=IN IP4 0.0.0.0\r\n"
        << "a=ice-ufrag:" << local_ufrag_ << "\r\n"
        << "a=ice-pwd:" << local_pwd_ << "\r\n"
        << "a=fingerprint:sha-256 " << fingerprint_ << "\r\n"
        << "a=setup:passive\r\n"
        << "a=mid:0\r\n"
        << "a=sendonly\r\n"
        << "a=rtcp-mux\r\n"
        << "a=rtcp-rsize\r\n"
        << "a=rtcp-fb:" << audio_payload_type_ << " transport-cc\r\n"
        << "a=rtcp-fb:" << audio_payload_type_ << " nack\r\n"
        << "a=ssrc:" << audio_ssrc_ << " cname:" << stream_name_ << "\r\n"
        << "a=ssrc:" << audio_ssrc_ << " msid:" << stream_name_ << " " << stream_name_ << "_audio\r\n"
        // << "a=ssrc:" << audio_ssrc_ << " mslabel:" << stream_name_ << "\r\n"
        // << "a=ssrc:" << audio_ssrc_ << " label:" << stream_name_ << "_audio\r\n"
        << "a=candidate:0 1 udp 2130706431 " << server_addr_ << " " << server_port_ << " typ host generation 0\r\n";
    }
    if (-1 != video_payload_type_)
    {
        oss << "m=video 9 UDP/TLS/RTP/SAVPF " << video_payload_type_ << "\r\n"
        << "a=rtpmap:" << video_payload_type_ << " H264/90000\r\n"
        // << "c=IN IP4 0.0.0.0\r\n"
        << "a=ice-ufrag:" << local_ufrag_ << "\r\n"
        << "a=ice-pwd:" << local_pwd_ << "\r\n"
        << "a=fingerprint:sha-256 " << fingerprint_ << "\r\n"
        << "a=setup:passive\r\n"
        << "a=mid:1\r\n"
        << "a=sendonly\r\n"
        << "a=rtcp-mux\r\n"
        << "a=rtcp-rsize\r\n"
        << "a=rtcp-fb:" << video_payload_type_ << " ccm fir\r\n"
        << "a=rtcp-fb:" << video_payload_type_ << " goog-remb\r\n"
        << "a=rtcp-fb:" << video_payload_type_ << " nack\r\n"
        << "a=rtcp-fb:" << video_payload_type_ << " nack pli\r\n"
        << "a=rtcp-fb:" << video_payload_type_ << " transport-cc\r\n"
        << "a=ssrc:" << video_ssrc_ << " cname:" << stream_name_ << "\r\n"
        << "a=ssrc:" << video_ssrc_ << " msid:" << stream_name_ << " " << stream_name_ << "_video\r\n"
        // << "a=ssrc:" << video_ssrc_ << " mslabel:" << stream_name_ << "\r\n"
        // << "a=ssrc:" << video_ssrc_ << " label:" << stream_name_ << "_video\r\n"
        << "a=candidate:0 1 udp 2130706431 " << server_addr_ << " " << server_port_ << " typ host generation 0\r\n";
    }

    return oss.str();
}

void SsmsSdp::SetStream(const std::string stream_name)
{
    stream_name_ = stream_name;
}

void SsmsSdp::SetFingerprint(std::string fingerprint)
{
    fingerprint_ = fingerprint;
}

std::string SsmsSdp::GetLocalUfrag() const
{
    return local_ufrag_;
}

std::string SsmsSdp::GetRemoteUfrag() const
{
    return remote_ufrag_;
}

std::string SsmsSdp::GetLocalPassword() const
{
    return local_pwd_;
}