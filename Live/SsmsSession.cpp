#include "SsmsSession.h"
#include "Network/SsmsTcpConnection.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Media/SsmsPlayClient.h"
#include "Media/SsmsPublishClient.h"
#include "Media/SsmsStream.h"

using namespace ssms::live;

SsmsSession::SsmsSession()
: stream_(std::make_shared<SsmsStream>())
{

}

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

void SsmsSession::SetProducer(const SsmsPublishClientPtr &producer)
{
    producer_ = producer;
}

SsmsStreamPtr SsmsSession::Stream() const
{
    return stream_;
}

void SsmsSession::ActiveAll()
{
    std::lock_guard<std::mutex> lk(lock_);
    for (auto it = consumers_.begin(); it != consumers_.end(); ++it)
    {
        (*it)->Active();
    }
}

void SsmsSession::DeActive(const SsmsPlayClientPtr &consumer)
{
    consumer->DeActive();
}