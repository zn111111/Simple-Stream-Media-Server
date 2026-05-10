#pragma once

#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
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
            void Run(const std::unordered_map<SsmsServerProtocol, SsmsNetAddressPtr> &local_addr_map,
                        const SsmsNetAddressPtr &udp_addr,
                        const SsmsLiveManagmentPtr &live_manage,
                        const SsmsWebrtcServerPtr &rtc_server);
            void Stop();
        private:
            void OnStart(int core_id);

            SsmsEventLoop *loop_{nullptr};
            std::thread thread_;
            SsmsTcpServerPtr tcp_server_;
            SsmsUdpServerPtr udp_server_;
            std::condition_variable condition_;
            std::mutex lock_;
            bool is_looping_{false};
            std::once_flag once_;
            std::promise<int> promise_;
        };
    }
}