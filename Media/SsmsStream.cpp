#include "SsmsStream.h"
#include "SsmsPacket.h"
#include "SsmsPlayClient.h"
#include "SsmsCodecHeader.h"
#include "Base/SsmsLogStream.h"
#include "SsmsGopManagment.h"

using namespace ssms::media;

SsmsStream::SsmsStream()
: codec_header(std::make_shared<SsmsCodecHeader>())
{
    packet_index_.store(0);
    gop_manage_ = std::make_shared<SsmsGopManagment>();
}

void SsmsStream::Push(const SsmsPacketPtr &pkt)
{
    std::lock_guard<std::mutex> lk(lock_);
    pkt->index_ = packet_index_.load();
    packet_index_.fetch_add(1);
    if (pkt->IsAudioSequenceHeader())
    {
        codec_header->PushAudioHeader(pkt);
    }
    else if (pkt->IsVideoSequenceHeader())
    {
        codec_header->PushVideoHeader(pkt);
    }
    else if (pkt->IsMeta())
    {
        codec_header->PushMetaHeader(pkt);
    }
    else
    {
        gop_manage_->PushPacket(pkt);
        packet_buffer_[pkt->index_ % packet_buffer_size_] = std::move(pkt);
    }
}

void SsmsStream::Pop(const SsmsPlayClientPtr &player)
{
    std::lock_guard<std::mutex> lk(lock_);
    int64_t idx = player->out_packet_index_ + 1;
    uint64_t max_idx = packet_index_.load();
    int content_latency = S_SSMSCONFIG->GetInteger("RTMP", "content_latency", 3) * 1000;

    //第一次给客户端发送数据, 获取头部并跳到I帧
    if (0 == idx)
    {
        player->meta_ = PopMetaHeader(idx);
        player->aac_sequence_header_ = PopAudioHeader(idx);
        player->avc_sequence_header_ = PopVideoHeader(idx);
        player->audio_header_version_++;
        player->video_header_version_++;
        player->meta_header_version_++;
        idx = gop_manage_->GetGopByLatency(content_latency);
    }
    //是否过期或延时过高, 是则清理过期gop并跳帧
    else if (idx <= ExpiredPacketIndex() || gop_manage_->LatestTimeStamp() - player->out_video_timestamp_ >= content_latency * 2)
    {
        int src_idx = idx;
        gop_manage_->ClearExpiredGop(ExpiredPacketIndex());
        idx = gop_manage_->GetGopByLatency(content_latency);
        LOG_DEBUG << "packet expired or latency is too high, latest video frame timestamp " << gop_manage_->LatestTimeStamp()
                    << ", client out latest video frame timestamp " << player->out_video_timestamp_
                    << ", expired packet index max value " << ExpiredPacketIndex() << ", client request packet index " << src_idx;
    }

    //头部更新则发新的头部
    if (codec_header->AudioHeaderVersion() > player->audio_header_version_)
    {
        player->aac_sequence_header_ = PopAudioHeader(idx);
        player->audio_header_version_++;
    }
    if (codec_header->VideoHeaderVersion() > player->video_header_version_)
    {
        player->avc_sequence_header_ = PopVideoHeader(idx);
        player->video_header_version_++;
    }
    if (codec_header->MetaHeaderVersion() > player->meta_header_version_)
    {
        player->meta_ = PopMetaHeader(idx);
        player->meta_header_version_++;
    }

    for (int i = 0; i < 30; i++)
    {
        if (idx >= max_idx)
        {
            break;
        }
        SsmsPacketPtr pkt = packet_buffer_[idx % packet_buffer_size_];
        if (pkt)
        {
            player->out_packet_.emplace_back(std::move(pkt));
        }
        idx++;
    }
    player->out_packet_index_ = idx - 1;
}

SsmsPacketPtr SsmsStream::PopAudioHeader(int index)
{
    return codec_header->PopAudioHeader(index);
}

SsmsPacketPtr SsmsStream::PopVideoHeader(int index)
{
    return codec_header->PopVideoHeader(index);
}

SsmsPacketPtr SsmsStream::PopMetaHeader(int index)
{
    return codec_header->PopMetaHeader(index);
}

int64_t SsmsStream::ExpiredPacketIndex() const
{
    return packet_index_.load() - 1 - packet_buffer_size_;
}