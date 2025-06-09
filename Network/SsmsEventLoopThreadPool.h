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

            void Start();
            SsmsEventLoopThreadPtr GetNextThread();
            uint32_t GetThreadNums() const;
        private:
            std::vector<SsmsEventLoopThreadPtr> threads_;
            uint32_t index_{0};
            uint32_t thread_nums_{0};
        };
    }
}