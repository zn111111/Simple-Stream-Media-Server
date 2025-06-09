#pragma once

#include <memory>
#include "SsmsContext.h"
#include "Base/SsmsBuffer.h"
#include "Network/SsmsTcpConnection.h"
#include "SsmsMediaDefine.h"

using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        //暂时只做简单握手
        class SsmsRtmpHandshakeContext
        {
        public:
            SsmsRtmpHandshakeContext(const TcpConnectionPtr &conn);
            ~SsmsRtmpHandshakeContext();

            //返回值小于0表示解析出错, 大于0表示数据不够, 等于0表示解析成功
            int Parse(const SsmsBufferPtr &data);
        private:
            int OnC0(const SsmsBufferPtr &data);
            int OnC1(const SsmsBufferPtr &data);
            int OnC2(const SsmsBufferPtr &data);
            void SendS0S1S2(bool has_c1, uint32_t timestamp);
            uint8_t GenRandom();

            RtmpHandshakeState state_{RtmpHandshakeWaitC0};
            std::weak_ptr<TcpConnection> conn_;
            char s0s1s2_[3073]{0};
        };
    }
}