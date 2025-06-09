#pragma once

#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include "Base/NonCopyable.h"

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
            ~SsmsEventLoopThread();

            SsmsEventLoop *Loop() const;
            void Run();
            void Stop();
        private:
            void OnStart();

            SsmsEventLoop *loop_{nullptr};
            std::thread thread_;
            std::condition_variable condition_;
            std::mutex lock_;
            bool is_looping_{false};
            std::once_flag once_;
            std::promise<int> promise_;
        };
    }
}