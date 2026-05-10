#pragma once

#include <unordered_map>
#include <string>
#include "Base/SsmsBuffer.h"
#include "SsmsMediaDefine.h"

using namespace ssms::base;

namespace ssms
{
    namespace media
    {
        namespace base
        {
            //http协议处理
            class SsmsHttpHandler
            {
            public:
                SsmsHttpHandler() = default;
                ~SsmsHttpHandler() = default;

                static int Parse(const SsmsBufferPtr &data, std::string &method, std::string &url, std::unordered_map<std::string, std::string> &out_header, const char **out_payload, int &out_payload_len);
            private:
                static int OnResponse(const std::string &method, const std::string &url, SsmsPacketPtr &pkt, const char *out_payload, int out_payload_len);                
            };
        }
    }
}