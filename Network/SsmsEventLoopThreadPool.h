#pragma once

#include <vector>
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

            void Start(const ssms::nw::SsmsNetAddressPtr &local_addr, const ssms::live::SsmsLiveManagmentPtr &live_manage);
            SsmsEventLoopThreadPtr GetNextThread();
            uint32_t GetThreadNums() const;
        private:
            std::vector<SsmsEventLoopThreadPtr> threads_;
            uint32_t index_{0};
            uint32_t thread_nums_{0};
        };
    }
}