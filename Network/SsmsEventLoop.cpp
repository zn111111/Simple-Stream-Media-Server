#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include "SsmsEventLoop.h"
#include "Base/SsmsLogStream.h"
#include "SsmsEvent.h"

using namespace ssms::nw;

static thread_local SsmsEventLoop *event_loop = nullptr;

SsmsEventLoop::SsmsEventLoop()
: epoll_fd_(::epoll_create(1024)), epoll_events_(1024)
{
    if (!event_loop)
    {
        event_loop = this;
    }
}

SsmsEventLoop::~SsmsEventLoop()
{
    Stop();
    if (epoll_fd_ > 0)
    {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void SsmsEventLoop::OnWork()
{
    looping_ = true;
    while (looping_)
    {
        //超时时间为1秒
        int ret = ::epoll_wait(epoll_fd_, &epoll_events_[0], epoll_events_.size(), 1000);
        if (ret >= 0)
        {
            for (int i = 0; i < ret; i++)
            {
                const struct epoll_event &ev = epoll_events_[i];
                if (ev.events & EPOLLERR)
                {
                    events_[ev.data.fd]->OnError();
                }
                else if ((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN))
                {
                    LOG_DEBUG << "peer closed the connection";
                    events_[ev.data.fd]->OnClose();
                }
                else if (ev.events & (EPOLLIN | EPOLLPRI))
                {
                    events_[ev.data.fd]->OnRead();
                }
                else if (ev.events & EPOLLOUT)
                {
                    events_[ev.data.fd]->OnWrite();
                }
            }

            //防止自动扩容发生拷贝
            if (epoll_events_.size() == ret)
            {
                epoll_events_.resize(ret * 2);
            }
        }
        else
        {
            LOG_ERROR << "epoll_wait error, " << strerror(errno);
        }

        //处理任务队列
        ProcessTask();
    }
}

void SsmsEventLoop::Stop()
{
    looping_ = false;
}

//EPOLLPRI: 带外数据
void SsmsEventLoop::AddEvent(const SsmsEventPtr &event)
{
    AssertInEventLoopThread();
    if (events_.find(event->Fd()) != events_.end())
    {
        LOG_DEBUG << "fd = " << event->Fd() << " has already existed";
        return;
    }
    events_[event->Fd()] = event;

    struct epoll_event epoll_event;
    epoll_event.events = EPOLLIN | EPOLLPRI | EPOLLET;
    event->SetEvent(epoll_event.events);
    epoll_event.data.fd = event->Fd();
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event->Fd(), &epoll_event) < 0)
    {
        LOG_ERROR << "epoll_ctl add error, " << strerror(errno) << ", fd = " << event->Fd();
        return;
    }
}

void SsmsEventLoop::DeleteEvent(int fd)
{
    AssertInEventLoopThread();
    if (events_.find(fd) == events_.end())
    {
        LOG_WARN << "fd = " << fd << " does not exist";
        return;
    }
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0)
    {
        LOG_ERROR << "epoll_ctl del error, " << strerror(errno) << ", fd = " << fd;
        return;
    }
    events_.erase(fd);
}

void SsmsEventLoop::AddTask(const TaskCallback &task)
{
    std::lock_guard<std::mutex> lk(lock_);
    tasks_.push(task);
}

void ssms::nw::SsmsEventLoop::AddTask(TaskCallback &&task)
{
    std::lock_guard<std::mutex> lk(lock_);
    tasks_.push(std::move(task));
}

void SsmsEventLoop::AssertInEventLoopThread()
{
    if (!IsInEventLoopThread())
    {
        LOG_ERROR << "is forbidden to run eventloop in other thread";
        exit(-1);
    }
}

bool SsmsEventLoop::IsInEventLoopThread()
{
    return this == event_loop;   
}

void SsmsEventLoop::EnableReadEvent(const SsmsEventPtr &event, bool enable)
{
    if (events_.find(event->fd_) == events_.end())
    {
        LOG_DEBUG << "fd is not exist";
        return;
    }

    struct epoll_event epoll_event;
    memset(&epoll_event, 0, sizeof(event));
    if (enable)
    {
        event->event_ |= EPOLLIN | EPOLLPRI | EPOLLET;
        ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &epoll_event);
    }
    else
    {
        event->event_ &= ~(EPOLLIN | EPOLLPRI | EPOLLET);
        ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &epoll_event);
    }
}

void SsmsEventLoop::EnableWriteEvent(const SsmsEventPtr &event, bool enable)
{
    if (events_.find(event->fd_) == events_.end())
    {
        LOG_DEBUG << "fd is not exist";
        return;
    }

    struct epoll_event epoll_event;
    memset(&epoll_event, 0, sizeof(epoll_event));
    epoll_event.data.fd = event->fd_;
    if (enable)
    {
        event->event_ |= EPOLLET | EPOLLOUT;
        epoll_event.events = event->event_;
        ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &epoll_event);
    }
    else
    {
        event->event_ &= ~(EPOLLET | EPOLLOUT);
        epoll_event.events = event->event_;
        ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &epoll_event);
    }
}

void SsmsEventLoop::ProcessTask()
{
    std::lock_guard<std::mutex> lk(lock_);
    while (!tasks_.empty())
    {
        auto &f = tasks_.front();
        f();
        tasks_.pop();
    }
}