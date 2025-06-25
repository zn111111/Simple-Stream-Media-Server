#include "SsmsTimestampCorrector.h"
#include "SsmsPacket.h"

using namespace ssms::media;

uint32_t SsmsTimestampCorrector::CorrectTimestamp(const SsmsPacketPtr &pkt)
{
    uint32_t ret = 0;
    if (pkt->IsAudio() && !pkt->IsAudioSequenceHeader())
    {
        ret = CorrectAudioByAudio(pkt->Timestamp());
    }
    else if (pkt->IsVideo() && !pkt->IsVideoSequenceHeader())
    {
        ret = CorrectVideoByVideo(pkt->Timestamp());
    }

    return ret;
}

uint32_t SsmsTimestampCorrector::CorrectAudioByAudio(uint32_t ori_timestamp)
{
    uint32_t ret = ori_timestamp;
    audio_num_between_video++;
    if (audio_num_between_video > 1)
    {
        int64_t delta = ori_timestamp - last_corrected_audio_timestamp_;
        if (delta <= 0 || delta > AUDIO_MAX_DELTA)
        {
            ret = last_corrected_audio_timestamp_ + AUDIO_DEFAULT_DELTA;
        }
        last_corrected_audio_timestamp_ = ret;
        last_audio_ori_timestamp_ = ori_timestamp;
    }
    else
    {
        if (last_audio_ori_timestamp_ < 0)
        {
            last_corrected_audio_timestamp_ = ret;
            last_audio_ori_timestamp_ = ori_timestamp;
        }
        else
        {
            ret = CorrectAudioByVideo(ori_timestamp);
        }
    }

    return ret;
}

uint32_t SsmsTimestampCorrector::CorrectAudioByVideo(uint32_t ori_timestamp)
{
    uint32_t ret = ori_timestamp;
    if (last_video_ori_timestamp_ < 0)
    {
        last_corrected_audio_timestamp_ = ret;
        last_audio_ori_timestamp_ = ori_timestamp;
        return ret;
    }

    int64_t delta = ori_timestamp - last_corrected_video_timestamp_;
    if (delta <= 0 || delta > VIDEO_MAX_DELTA)
    {
        ret = last_corrected_video_timestamp_ + VIDEO_DEFAULT_DELTA;
    }
    last_corrected_audio_timestamp_ = ret;
    last_audio_ori_timestamp_ = ori_timestamp;

    return ret;
}

uint32_t SsmsTimestampCorrector::CorrectVideoByVideo(uint32_t ori_timestamp)
{
    audio_num_between_video = 0;
    uint32_t ret = ori_timestamp;
    if (last_video_ori_timestamp_ < 0)
    {
        last_video_ori_timestamp_ = ori_timestamp;
        last_corrected_video_timestamp_ = ret;
        int64_t delta = ori_timestamp - last_corrected_audio_timestamp_;    
        if (last_corrected_audio_timestamp_ >= 0 && (delta < -VIDEO_MAX_DELTA || delta > VIDEO_MAX_DELTA))
        {
            last_video_ori_timestamp_ = ori_timestamp;
            ret = last_corrected_audio_timestamp_;
            last_corrected_video_timestamp_ = ret;
        }
        return ret;
    }

    int64_t delta = ori_timestamp - last_corrected_video_timestamp_;
    if (delta <= 0 || delta > VIDEO_MAX_DELTA)
    {
        ret = last_corrected_video_timestamp_ + VIDEO_DEFAULT_DELTA;
    }
    last_video_ori_timestamp_ = ori_timestamp;
    last_corrected_video_timestamp_ = ret;

    return ret;
}