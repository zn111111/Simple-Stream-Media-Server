#pragma once

#include <vector>
#include <unordered_map>
#include "Base/NonCopyable.h"
#include "SsmsNetworkDefine.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoopThreadPool : public NonCopyable
        {
        public:
            SsmsEventLoopThreadPool();
            ~SsmsEventLoopThreadPool();

            void Start(const std::unordered_map<SsmsServerProtocol, SsmsNetAddressPtr> &local_addr_map,
                        const SsmsNetAddressPtr &udp_addr,
                        const SsmsLiveManagmentPtr &live_manage,
                        const SsmsWebrtcServerPtr &rtc_server);
            SsmsEventLoopThreadPtr GetNextThread();
            uint32_t GetThreadNums() const;
        private:
            std::vector<SsmsEventLoopThreadPtr> threads_;
            uint32_t index_{0};
            uint32_t thread_nums_{0};
        };
    }
}