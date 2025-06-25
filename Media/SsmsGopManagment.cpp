#include "SsmsGopManagment.h"
#include "SsmsPacket.h"

using namespace ssms::media;

void SsmsGopManagment::PushPacket(const SsmsPacketPtr &pkt)
{
    if (pkt->IsKeyFrame())
    {
        gops_.emplace_back(pkt->index_, pkt->Timestamp());
        latest_timestamp_ = pkt->Timestamp();
    }
    else if (pkt->IsVideo() && !pkt->IsVideoSequenceHeader())
    {
        latest_timestamp_ = pkt->Timestamp();
    }
}

int SsmsGopManagment::GetGopByLatency(int content_latency)
{
    int ret = 0;
    for (int i = gops_.size() - 1; i >= 0; i--)
    {
        if (latest_timestamp_ - gops_[i].key_frame_timestamp <= content_latency)
        {
            ret = gops_[i].key_frame_index;
        }
    }

    return ret;
}

void SsmsGopManagment::ClearExpiredGop(int min_index)
{
    for (auto it = gops_.begin(); it != gops_.end();)
    {
        if (it->key_frame_index <= min_index)
        {
            it = gops_.erase(it);
        }
        else
        {
            break;
        }
    }
}

uint32_t SsmsGopManagment::LatestTimeStamp()
{
    return latest_timestamp_;
}