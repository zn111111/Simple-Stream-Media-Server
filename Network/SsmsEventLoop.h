#pragma once

#include <vector>
#include <sys/epoll.h>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <functional>
#include <list>
#include "Base/NonCopyable.h"
#include "SsmsNetworkDefine.h"
#include "Base/SsmsTimingWheel.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoop : public NonCopyable
        {
        public:
            SsmsEventLoop();
            ~SsmsEventLoop();

            void OnWork();
            void Stop();
            //添加读事件
            void AddEvent(const SsmsEventPtr &event);
            //删除事件
            void DeleteEvent(int fd);
            void AddTask(const TaskCallback &task);
            void AddTask(TaskCallback &&task);
            //当前线程是否是事件循环所在的线程
            bool IsInEventLoopThread();
            void EnableReadEvent(const SsmsEventPtr &event, bool enable);                      //开启或关闭读事件
            void EnableWriteEvent(const SsmsEventPtr &event, bool enable);                     //开启或关闭写事件
            //添加定时任务
            void RunAfter(uint32_t seconds, TimingCallback func);
            void RunEvery(uint32_t seconds, TimingCallback func);
        private:
            //当前线程是事件循环所在的线程
            void AssertInEventLoopThread();
            void ProcessTask();

            bool looping_{false};
            int epoll_fd_{-1};
            std::vector<struct epoll_event> epoll_events_;
            std::unordered_map<int, SsmsEventPtr> events_;
            //任务队列锁
            std::mutex lock_;
            //任务队列
            std::queue<TaskCallback> tasks_;
            std::list<TaskCallback> tasks_copy_;
            //定时器
            SsmsTimingWheelPtr timer_;
        };
    }
}