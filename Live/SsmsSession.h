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
            SsmsSession();
            ~SsmsSession() = default;

            void AddConsumer(const SsmsPlayClientPtr &consumer);
            void AddConsumer(const SsmsWebrtcPlayClientPtr &consumer);
            void DeleteConsumer(const SsmsPlayClientPtr &consumer);
            void DeleteConsumer(const SsmsWebrtcPlayClientPtr &consumer);
            void SetProducer(const SsmsPublishClientPtr &producer);
            SsmsStreamPtr Stream() const;
            void ActiveAll();
            void DeActive(const SsmsPlayClientPtr &consumer);
        private:
            std::mutex tcp_lock_;
            std::unordered_set<SsmsPlayClientPtr> tcp_consumers_;
            std::mutex udp_lock_;
            std::unordered_set<SsmsWebrtcPlayClientPtr> udp_consumers_;
            SsmsPublishClientPtr producer_;
            SsmsStreamPtr stream_;
        };
    }
}