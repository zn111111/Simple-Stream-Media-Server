#include "SsmsSession.h"
#include "Network/SsmsTcpConnection.h"
#include "Media/SsmsRtmpMessageContext.h"
#include "Media/SsmsPlayClient.h"
#include "Media/SsmsPublishClient.h"
#include "Media/SsmsStream.h"
#include "Media/SsmsWebrtcPlayClient.h"

using namespace ssms::live;

SsmsSession::SsmsSession()
: stream_(std::make_shared<SsmsStream>())
{

}

void SsmsSession::AddConsumer(const SsmsPlayClientPtr &consumer)
{
    std::lock_guard<std::mutex> lk(tcp_lock_);
    if (tcp_consumers_.find(consumer) != tcp_consumers_.end())
    {
        return;
    }

    tcp_consumers_.insert(consumer);
}

void SsmsSession::AddConsumer(const SsmsWebrtcPlayClientPtr &consumer)
{
    std::lock_guard<std::mutex> lk(udp_lock_);
    if (udp_consumers_.find(consumer) != udp_consumers_.end())
    {
        return;
    }

    udp_consumers_.insert(consumer);
}

void SsmsSession::DeleteConsumer(const SsmsPlayClientPtr &consumer)
{
    std::lock_guard<std::mutex> lk(tcp_lock_);
    if (tcp_consumers_.find(consumer) == tcp_consumers_.end())
    {
        return;
    }

    tcp_consumers_.erase(consumer);
}

void SsmsSession::DeleteConsumer(const SsmsWebrtcPlayClientPtr &consumer)
{
    std::lock_guard<std::mutex> lk(udp_lock_);
    if (udp_consumers_.find(consumer) == udp_consumers_.end())
    {
        return;
    }

    udp_consumers_.erase(consumer);
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
    {
        //TODO
        //这里激活时会去获取任务队列的锁, 有可能会出现死锁, 但是任务队列那里已经做了处理, 破坏了死锁的条件, 因此不会出现死锁
        //这边不一块做处理是因为如果使用容器先把consumer存起来再无锁处理consumer, 可能出现其他线程已经删除了consumer而这边
        //还在处理consumer的情况, 因为consumer的所有权无法转移, 未被删除前必须存在consumers_里
        std::lock_guard<std::mutex> lk(tcp_lock_);
        for (auto it = tcp_consumers_.begin(); it != tcp_consumers_.end(); ++it)
        {
            (*it)->Active();
        }
    }

    {
        std::lock_guard<std::mutex> lk(udp_lock_);
        for (auto it = udp_consumers_.begin(); it != udp_consumers_.end(); ++it)
        {
            (*it)->Active();
        }
    }
}

void SsmsSession::DeActive(const SsmsPlayClientPtr &consumer)
{
    consumer->DeActive();
}