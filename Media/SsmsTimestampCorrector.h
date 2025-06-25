#pragma once

#include <stdint.h>
#include "SsmsMediaDefine.h"

namespace ssms
{
    namespace media
    {
        class SsmsTimestampCorrector
        {
        public:
            SsmsTimestampCorrector() = default;
            ~SsmsTimestampCorrector() = default;

            uint32_t CorrectTimestamp(const SsmsPacketPtr &pkt);
        private:
            uint32_t CorrectAudioByAudio(uint32_t ori_timestamp);
            uint32_t CorrectAudioByVideo(uint32_t ori_timestamp);
            uint32_t CorrectVideoByVideo(uint32_t ori_timestamp);

            //上一个音频原始时间戳
            int64_t last_audio_ori_timestamp_{-1};
            //上一个纠正后的音频时间戳
            int64_t last_corrected_audio_timestamp_{-1};
            //上一个视频原始时间戳
            int64_t last_video_ori_timestamp_{-1};
            //上一个纠正后的视频时间戳
            int64_t last_corrected_video_timestamp_{-1};
            //两个视频之间的音频数
            int audio_num_between_video{0};
        };
    }
}