#include <sys/time.h>
#include "SsmsTimingWheel.h"

static const int WHEEL_TYPE_NUMS = 4;
static const int MINUTE_SEOCND_NUMS = 60;
static const int HOUR_SEOCND_NUMS = 60 * 60;
static const int DAY_SEOCND_NUMS = 60 * 60 * 24;

SsmsTimingWheel::SsmsTimingWheel()
: wheels_(WHEEL_TYPE_NUMS)
{
    wheels_[WheelTypeSecond].resize(60);
    wheels_[WheelTypeMinute].resize(60);
    wheels_[WheelTypeHour].resize(24);
    wheels_[WheelTypeDay].resize(30);
}

SsmsTimingWheel::~SsmsTimingWheel()
{

}

void SsmsTimingWheel::OnTiming()
{
    struct timeval tv{0, 0};
    gettimeofday(&tv, nullptr);
    uint64_t curr_ts = tv.tv_sec * 1000;
    if (curr_ts - last_ts_ < 1000)
    {
        return;
    }

    last_ts_ = curr_ts;
    counts_++;
    wheels_[WheelTypeSecond].pop_front();
    wheels_[WheelTypeSecond].emplace_back(Tasks());
    if (0 == counts_ % MINUTE_SEOCND_NUMS)
    {
        wheels_[WheelTypeMinute].pop_front();
        wheels_[WheelTypeMinute].emplace_back(Tasks());
    }
    if (0 == counts_ % HOUR_SEOCND_NUMS)
    {
        wheels_[WheelTypeHour].pop_front();
        wheels_[WheelTypeHour].emplace_back(Tasks());
    }
    if (0 == counts_ % DAY_SEOCND_NUMS)
    {
        wheels_[WheelTypeDay].pop_front();
        wheels_[WheelTypeDay].emplace_back(Tasks());
    }
}

void SsmsTimingWheel::RunAfter(uint32_t seconds, TimingCallback func)
{
    if (0 == seconds)
    {
        func();
        return;
    }

    if (seconds < MINUTE_SEOCND_NUMS)
    {
        InsertSeconds(seconds, func);
    }
    else if (seconds < HOUR_SEOCND_NUMS)
    {
        InsertMinutes(seconds, func);
    }
    else if (seconds < DAY_SEOCND_NUMS)
    {
        InsertHours(seconds, func);
    }
    else
    {
        InsertDays(seconds, func);
    }
}

//flag变为0表示达到了某个条件需要退出
void SsmsTimingWheel::RunEvery(uint32_t seconds, TimingCallback func)
{
    RunAfter(seconds, [this, seconds, func] () {
        func();
        RunEvery(seconds, func);
    });
}

void SsmsTimingWheel::InsertSeconds(uint32_t seconds, TimingCallback func)
{
    wheels_[WheelTypeSecond][seconds].emplace_back(TimingTask(std::move(func)));
}

void SsmsTimingWheel::InsertMinutes(uint32_t seconds, TimingCallback func)
{
    uint32_t min = seconds / MINUTE_SEOCND_NUMS;
    uint32_t extra_sec = seconds - min * MINUTE_SEOCND_NUMS;
    TimingTask task([this, extra_sec, func] () {
        RunAfter(extra_sec, func);
    });
    wheels_[WheelTypeMinute][min].emplace_back(std::move(task));
}

void SsmsTimingWheel::InsertHours(uint32_t seconds, TimingCallback func)
{
    uint32_t hour = seconds / HOUR_SEOCND_NUMS;
    uint32_t extra_sec = seconds - hour * HOUR_SEOCND_NUMS;
    TimingTask task([this, extra_sec, func] () {
        RunAfter(extra_sec, func);
    });
    wheels_[WheelTypeHour][hour].emplace_back(std::move(task));
}

void SsmsTimingWheel::InsertDays(uint32_t seconds, TimingCallback func)
{
    uint32_t day = seconds / DAY_SEOCND_NUMS;
    uint32_t extra_sec = seconds - day * DAY_SEOCND_NUMS;
    TimingTask task([this, extra_sec, func] () {
        RunAfter(extra_sec, func);
    });
    wheels_[WheelTypeDay][day].emplace_back(std::move(task));
}