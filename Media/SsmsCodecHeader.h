#pragma once

#include <vector>
#include "SsmsPacket.h"

namespace ssms
{
    namespace media
    {
        class SsmsCodecHeader
        {
        public:
            SsmsCodecHeader() = default;
            ~SsmsCodecHeader() = default;

            void PushAudioHeader(const SsmsPacketPtr &pkt);
            void PushVideoHeader(const SsmsPacketPtr &pkt);
            void PushMetaHeader(const SsmsPacketPtr &pkt);
            //查找索引大于index的最小索引对应的音频头部
            SsmsPacketPtr PopAudioHeader(int index);
            SsmsPacketPtr PopVideoHeader(int index);
            SsmsPacketPtr PopMetaHeader(int index);
            int AudioHeaderVersion();
            int VideoHeaderVersion();
            int MetaHeaderVersion();
        private:
            std::vector<SsmsPacketPtr> audio_headers_;
            std::vector<SsmsPacketPtr> video_headers_;
            std::vector<SsmsPacketPtr> meta_headers_;

            int audio_header_version_{0};
            int video_header_version_{0};
            int meta_header_version_{0};
        };
    }
}