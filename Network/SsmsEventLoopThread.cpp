#include "SsmsEventLoopThread.h"
#include "SsmsEventLoop.h"
#include "SsmsTcpServer.h"
#include "Base/SsmsLogStream.h"

using namespace ssms::nw;

SsmsEventLoopThread::SsmsEventLoopThread()
{

}

SsmsEventLoopThread::SsmsEventLoopThread(int core_id)
: thread_([this, core_id] () {OnStart(core_id);})
{

}

//调用Run是防止Run从未被外界调用过, 导致线程一直阻塞在
//条件变量condition_处, 导致析构函数一直阻塞, 等待回收
//线程, 导致线程无法退出
SsmsEventLoopThread::~SsmsEventLoopThread()
{
    Run(nullptr, nullptr);
    if (loop_)
    {
        loop_->Stop();
        loop_ = nullptr;
    }

    if (thread_.joinable())
    {
        thread_.join();
    }
}

SsmsEventLoop *SsmsEventLoopThread::Loop() const
{
    return loop_;
}

//在多线程中, SsmsEventLoop的指针是会被其他线程获取到的
//因此在SsmsEventLoop指针的赋值的过程中需要确保线程同步
//Run运行完直接退出可能导致OnStart里的loop_ = &loop赋值
//还没有完成, 导致其他线程获取到的SsmsEventLoop指针为空
//因此, 这里使用promise_阻塞等待OnStart里调用set_value设置值
void SsmsEventLoopThread::Run(const SsmsNetAddressPtr &local_addr, const SsmsLiveManagmentPtr &live_manage)
{
    std::call_once(once_, [this, local_addr, live_manage] () {
        {
            std::lock_guard<std::mutex> lk(lock_);
            is_looping_ = true;
            condition_.notify_all();
        }
        auto f = promise_.get_future();
        f.get();

        tcp_server_ = std::make_shared<SsmsTcpServer>(loop_, local_addr, live_manage);
        tcp_server_->Start();
    });
}

void SsmsEventLoopThread::Stop()
{
    loop_->Stop();
}

void SsmsEventLoopThread::OnStart(int core_id)
{
    //设置cpu亲和性
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset))
    {
        LOG_ERROR << "thread "<< pthread_self() << " set cpu affinity error";
        exit(1);
    }

    std::unique_lock<std::mutex> lk(lock_);
    condition_.wait(lk, [this] () {return is_looping_;});
    SsmsEventLoop loop;
    loop_ = &loop;
    promise_.set_value(1);
    loop.OnWork();
    is_looping_ = false;
}