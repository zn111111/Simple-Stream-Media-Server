#include "SsmsEventLoopThreadPool.h"
#include "Base/SsmsINIReader.h"
#include "SsmsEventLoopThread.h"

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
    for (int i = 0; i < thread_nums_; i++)
    {
        SsmsEventLoopThreadPtr thread = std::make_shared<SsmsEventLoopThread>();
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