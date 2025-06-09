#include "SsmsSession.h"
#include "Network/SsmsTcpConnection.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Media/SsmsPlayClient.h"
#include "Media/SsmsPublishClient.h"

using namespace ssms::live;

void SsmsSession::AddConsumer(const SsmsPlayClientPtr &consumer)
{
    std::lock_guard<std::mutex> lk(lock_);
    if (consumers_.find(consumer) != consumers_.end())
    {
        return;
    }

    consumers_.insert(consumer);
}

void SsmsSession::DeleteConsumer(const SsmsPlayClientPtr &consumer)
{
    std::lock_guard<std::mutex> lk(lock_);
    if (consumers_.find(consumer) == consumers_.end())
    {
        return;
    }

    consumers_.erase(consumer);
}

void SsmsSession::SendDataToConsumers(const SsmsPacketPtr &data)
{
    std::lock_guard<std::mutex> lk(lock_);
    for (auto it = consumers_.begin(); it != consumers_.end(); ++it)
    {
        if ((*it)->NewComming())
        {
            if (producer_->Meta())
            {
                (*it)->Play(producer_->Meta(), true);
            }
            if (producer_->AudioSequenceHeader())
            {
                (*it)->Play(producer_->AudioSequenceHeader(), true);
            }
            if (producer_->VideoSequenceHeader())
            {
                (*it)->Play(producer_->VideoSequenceHeader(), true);
            }
            (*it)->SetToOld();
        }
        (*it)->Play(data, true);
    }
}

void SsmsSession::SetProducer(const SsmsPublishClientPtr &producer)
{
    producer_ = producer;
}