#pragma once

#include <vector>
#include "SsmsMediaDefine.h"

namespace ssms
{
    namespace media
    {
        class SsmsGopManagment
        {
        public:
            SsmsGopManagment() = default;
            ~SsmsGopManagment() = default;

            void PushPacket(const SsmsPacketPtr &pkt);
            int GetGopByLatency(int content_latency);
            void ClearExpiredGop(int min_index);
            //最新的视频帧的时间戳
            uint32_t LatestTimeStamp();
        private:
            std::vector<struct GopItemInfo> gops_;
            uint32_t latest_timestamp_{0};
        };
    }
}