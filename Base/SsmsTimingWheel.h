#pragma once

#include <vector>
#include <deque>
#include <list>
#include <functional>
#include <memory>
#include "NonCopyable.h"

using namespace ssms::base;

enum WheelType
{
    WheelTypeSecond,        //秒
    WheelTypeMinute,        //分钟
    WheelTypeHour,          //小时
    WheelTypeDay            //天
};

using TimingCallback = std::function<void ()>;
class TimingTask
{
public:
    TimingTask(TimingTask &&task)
    : func_(std::move(task.func_))
    {

    }
    TimingTask(TimingCallback &&func)
    : func_(std::move(func))
    {

    }

    ~TimingTask()
    {
        if (func_)
        {
            func_();
        }
    }
private:
    TimingCallback func_;
};

using Tasks = std::list<TimingTask>;
using Wheels = std::deque<Tasks>;
class SsmsTimingWheel;
using SsmsTimingWheelPtr = std::unique_ptr<SsmsTimingWheel>;
class SsmsTimingWheel : public NonCopyable
{
public:
    SsmsTimingWheel();
    ~SsmsTimingWheel();

    void OnTiming();
    void RunAfter(uint32_t seconds, TimingCallback func);
    void RunEvery(uint32_t seconds, TimingCallback func);
private:
    void InsertSeconds(uint32_t seconds, TimingCallback func);
    void InsertMinutes(uint32_t seconds, TimingCallback func);
    void InsertHours(uint32_t seconds, TimingCallback func);
    void InsertDays(uint32_t seconds, TimingCallback func);

    std::vector<Wheels> wheels_;
    uint64_t last_ts_{0};
    //时间轮转动的次数, 1次表示1秒
    uint64_t counts_{0};
};