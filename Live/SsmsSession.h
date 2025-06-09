#pragma once

#include <unordered_set>
#include <mutex>
#include "Base/NonCopyable.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::base;
using namespace ssms::nw;

namespace ssms
{
    namespace live
    {
        class SsmsSession : public NonCopyable
        {
        public:
            SsmsSession() = default;
            ~SsmsSession() = default;

            void AddConsumer(const SsmsPlayClientPtr &consumer);
            void DeleteConsumer(const SsmsPlayClientPtr &consumer);
            void SendDataToConsumers(const SsmsPacketPtr &data);
            void SetProducer(const SsmsPublishClientPtr &producer);
        private:
            std::mutex lock_;
            std::unordered_set<SsmsPlayClientPtr> consumers_;
            SsmsPublishClientPtr producer_;
        };
    }
}