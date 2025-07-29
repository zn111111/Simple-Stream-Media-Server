#include "SsmsEventLoopThreadPool.h"
#include "Base/SsmsINIReader.h"
#include "SsmsEventLoopThread.h"
#include "Base/SsmsLogStream.h"
#include "Base/SsmsUtils.h"

using namespace ssms::nw;

SsmsEventLoopThreadPool::SsmsEventLoopThreadPool()
: threads_(S_SSMSCONFIG->GetInteger("COMMON", "thread_nums", 4))
, thread_nums_(threads_.size())
{

}

SsmsEventLoopThreadPool::~SsmsEventLoopThreadPool()
{

}

void SsmsEventLoopThreadPool::Start(const ssms::nw::SsmsNetAddressPtr &local_addr, const ssms::live::SsmsLiveManagmentPtr &live_manage)
{
    std::string cpu = S_SSMSCONFIG->GetString("COMMON", "running_cpu", "");
    if (cpu.empty())
    {
        LOG_ERROR << "COMMON running_cpu is empty";
        exit(1);
    }
    std::vector<int> v_cpu = SsmsUtils::Split(cpu, ",");
    if (v_cpu.empty() || v_cpu.size() != thread_nums_)
    {
        LOG_ERROR << "no cpu is set, or cpu nums is not equal to thread nums";
        exit(1);
    }

    for (int i = 0; i < thread_nums_; i++)
    {
        SsmsEventLoopThreadPtr thread = std::make_shared<SsmsEventLoopThread>(v_cpu[i]);
        thread->Run(local_addr, live_manage);
        threads_[i] = thread;
    }
}

SsmsEventLoopThreadPtr SsmsEventLoopThreadPool::GetNextThread()
{
    return threads_[index_++ % thread_nums_];
}

uint32_t SsmsEventLoopThreadPool::GetThreadNums() const
{
    return thread_nums_;
}