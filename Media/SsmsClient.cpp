#include "SsmsClient.h"

using namespace ssms::media;

SsmsClient::SsmsClient(const std::string &app,
                const std::string &stream,
                const SsmsLiveManagmentPtr &live_manage,
                const TcpConnectionPtr &conn,
                SsmsRtmpMessageContextPtr context,
                SsmsEventLoop *loop)
: app_name_(app)
, stream_name_(stream)
, live_manage_(live_manage)
, conn_(conn)
, context_(context)
, loop_(loop)
{

}