#include "SsmsClient.h"

using namespace ssms::media;

SsmsClient::SsmsClient(const std::string &app,
                const std::string &stream,
                const SsmsLiveManagmentPtr &live_manage,
                const TcpConnectionPtr &conn,
                SsmsContextPtr context,
                SsmsEventLoop *loop)
: app_name_(app)
, stream_name_(stream)
, live_manage_(live_manage)
, conn_(conn)
, context_(context)
, loop_(loop)
{

}

int SsmsClient::Process(const SsmsPacketPtr &data, const std::string &command, double trans_id)
{
    return 0;
}

int SsmsClient::Process(const std::string &method, const std::string &url, const char *out_payload, int out_payload_len, SsmsPacketPtr &pkt)
{
    return 0;
}

int SsmsClient::Init()
{
    return 0;
}