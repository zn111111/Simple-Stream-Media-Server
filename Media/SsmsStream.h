#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include "SsmsMediaDefine.h"

namespace ssms
{
    namespace media
    {
        class SsmsStream
        {
            friend class SsmsPlayClient;
        public:
            SsmsStream();
            ~SsmsStream() = default;

            void Push(const SsmsPacketPtr &pkt);
            void Pop(const SsmsPlayClientPtr &player);
            //过期的包的索引的最大值, 小于等于该索引值的包都已经过期
            int64_t ExpiredPacketIndex() const;
        private:
            //查找索引大于index的最小索引对应的音频头部
            SsmsPacketPtr PopAudioHeader(int index);
            SsmsPacketPtr PopVideoHeader(int index);
            SsmsPacketPtr PopMetaHeader(int index);

            std::mutex lock_;
            std::vector<SsmsPacketPtr> packet_buffer_{1000};
            uint32_t packet_buffer_size_{1000};
            //每个packet一个唯一的索引, 递增
            std::atomic<uint64_t> packet_index_;
            SsmsCodecHeaderPtr codec_header;
            SsmsGopManagmentPtr gop_manage_;
        };
    }
}