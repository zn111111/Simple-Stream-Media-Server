#include "SsmsHttpMessageContext.h"
#include "Base/SsmsLogStream.h"
#include "Network/SsmsTcpConnection.h"
#include "SsmsClient.h"
#include "SsmsWebrtcPlayClient.h"
#include "Live/SsmsLiveManagment.h"
#include "Live/SsmsSession.h"
#include "Network/SsmsNetAddress.h"

using namespace ssms::base;
using namespace ssms::nw;

SsmsHttpMessageContext::SsmsHttpMessageContext(SsmsEventLoop *loop,
                                            TcpConnectionPtr conn,
                                            SsmsLiveManagmentPtr live_manage,
                                            SsmsWebrtcServerPtr rtc_server)
: loop_(loop)
, conn_(conn)
, rtc_server_(rtc_server)
{
    SsmsContext::live_manage_ = live_manage;
}

int SsmsHttpMessageContext::Parse(const SsmsBufferPtr &data)
{
    std::string method, url;
    std::unordered_map<std::string, std::string> out_header;
    const char *out_payload;
    int out_payload_len;
    int ret = 0;
    if (ret = SsmsHttpHandler::Parse(data, method, url, out_header, &out_payload, out_payload_len))
    {
        return ret;
    }

    SsmsPacketPtr pkt;
    if ("/rtc/v1/play/" == url)
    {
        if ("OPTIONS" == method)
        {
            std::stringstream stream;
            stream << "HTTP/1.1 200 OK\r\n"
            << "Server: ssms\r\n"
            << "Content-length: 0\r\n"
            << "Content-type: text/plain\r\n"
            << "Access-Control-Allow-Origin: *\r\n"
            << "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
            << "Allow: POST, GET, OPTIONS*\r\n"
            << "Access-Control-Allow-Headers: Content-type\r\n\r\n";
            pkt = std::make_shared<SsmsPacket>(stream.str().size());
            memcpy(pkt->data, stream.str().c_str(), stream.str().size());
        }
        else if ("POST" == method)
        {
            std::string local_ip = S_SSMSCONFIG->GetString("WEBRTC", "server_ip", "");
            std::string local_port = S_SSMSCONFIG->GetString("WEBRTC", "server_port", "");
            if (local_ip.empty() || local_port.empty())
            {
                LOG_ERROR << "load config failed, WEBRTC.server_ip or WEBRTC.server_port is null";
                exit(1);
            }
            SsmsNetAddressPtr server_addr = std::make_shared<SsmsNetAddress>(local_ip, atoi(local_port.c_str()), true);
            SsmsWebrtcPlayClientPtr rtc_client = std::make_shared<SsmsWebrtcPlayClient>(live_manage_,
                                                                                        loop_,
                                                                                        server_addr,
                                                                                        rtc_server_);
            if (rtc_client->Init() < 0)
            {
                LOG_ERROR << "SsmsWebrtcPlayClient init failed";
                ret = -1;
                return ret;
            }
            if ((ret = rtc_client->ProcessSdp(method, out_payload, out_payload_len, pkt)) < 0)
            {
                LOG_ERROR << "process sdp message failed";
                ret = -1;
                return ret;
            }
        }
        else
        {
            LOG_ERROR << "url: "<< url<< ", unsupported http method: " << method;
            ret = -1;
            return ret;    
        }
    }
    else
    {
        LOG_ERROR << "unsupported http url: " << url;
        ret = -1;
        return ret;
    }
    struct iovec node;
    node.iov_base = pkt->data;
    node.iov_len = pkt->payload_size_;
    sending_pkts_.emplace_back(std::move(pkt));
    conn_.lock()->SendNode(node);

    return ret;
}

void SsmsHttpMessageContext::ClearSendCompleteData()
{
    sending_pkts_.clear();
}