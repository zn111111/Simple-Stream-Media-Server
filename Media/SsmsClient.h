#pragma once

#include <string>
#include <memory>
#include "SsmsMediaDefine.h"
#include "Live/SsmsLiveDefine.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::live;
using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsClient : public std::enable_shared_from_this<SsmsClient>
        {
        public:
            SsmsClient(const std::string &app,
                            const std::string &stream,
                            const SsmsLiveManagmentPtr &live_manage,
                            const TcpConnectionPtr &conn,
                            SsmsContextPtr context,
                            SsmsEventLoop *loop);
            virtual ~SsmsClient() = default;
            
            virtual int Process(const SsmsPacketPtr &data, const std::string &command, double trans_id = 999999.999999);
            virtual int Process(const std::string &method, const std::string &url, const char *out_payload, int out_payload_len, SsmsPacketPtr &pkt);
            virtual int Init();
        protected:
            std::string app_name_;
            std::string stream_name_;
            SsmsSessionPtr sess_;
            SsmsLiveManagmentPtr live_manage_;
            TcpConnectionPtr conn_;
            SsmsContextPtr context_;
            SsmsEventLoop *loop_{nullptr};
        };
    }
}