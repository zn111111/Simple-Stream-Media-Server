#pragma once

#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include "Base/NonCopyable.h"
#include "SsmsNetworkDefine.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoop;
        class SsmsEventLoopThread : public NonCopyable
        {
        public:
            SsmsEventLoopThread();
            SsmsEventLoopThread(int core_id);
            ~SsmsEventLoopThread();

            SsmsEventLoop *Loop() const;
            void Run(const SsmsNetAddressPtr &local_addr, const SsmsLiveManagmentPtr &live_manage);
            void Stop();
        private:
            void OnStart(int core_id);

            SsmsEventLoop *loop_{nullptr};
            std::thread thread_;
            SsmsTcpServerPtr tcp_server_;
            std::condition_variable condition_;
            std::mutex lock_;
            bool is_looping_{false};
            std::once_flag once_;
            std::promise<int> promise_;
        };
    }
}