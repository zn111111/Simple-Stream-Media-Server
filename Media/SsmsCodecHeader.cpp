#include "SsmsCodecHeader.h"

using namespace ssms::media;

void SsmsCodecHeader::PushAudioHeader(const SsmsPacketPtr &pkt)
{
    audio_headers_.emplace_back(std::move(pkt));
    audio_header_version_++;
}

void SsmsCodecHeader::PushVideoHeader(const SsmsPacketPtr &pkt)
{
    video_headers_.emplace_back(std::move(pkt));
    video_header_version_++;
}

void SsmsCodecHeader::PushMetaHeader(const SsmsPacketPtr &pkt)
{
    meta_headers_.emplace_back(std::move(pkt));
    meta_header_version_++;
}

SsmsPacketPtr SsmsCodecHeader::PopAudioHeader(int index)
{
    if (audio_headers_.empty())
    {
        return nullptr;
    }

    int res = 0;
    for (int i = audio_headers_.size() - 1; i >= 0; i--)
    {
        if (audio_headers_[i]->index_ < index)
        {
            break;
        }
        res = i;
    }

    return audio_headers_[res];
}

SsmsPacketPtr SsmsCodecHeader::PopVideoHeader(int index)
{
    if (video_headers_.empty())
    {
        return nullptr;
    }

    int res = 0;
    for (int i = video_headers_.size() - 1; i >= 0; i--)
    {
        if (video_headers_[i]->index_ < index)
        {
            break;
        }
        res = i;
    }

    return video_headers_[res];
}

SsmsPacketPtr SsmsCodecHeader::PopMetaHeader(int index)
{
    if (meta_headers_.empty())
    {
        return nullptr;
    }

    int res = 0;
    for (int i = meta_headers_.size() - 1; i >= 0; i--)
    {
        if (meta_headers_[i]->index_ < index)
        {
            break;
        }
        res = i;
    }

    return meta_headers_[res];
}

int SsmsCodecHeader::AudioHeaderVersion()
{
    return audio_header_version_;
}

int SsmsCodecHeader::VideoHeaderVersion()
{
    return video_header_version_;
}

int SsmsCodecHeader::MetaHeaderVersion()
{
    return meta_header_version_;
}