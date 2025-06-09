#include <unistd.h>
#include <sys/epoll.h>
#include "SsmsEvent.h"

using namespace ssms::nw;

SsmsEvent::SsmsEvent(int fd)
: fd_(fd)
{

}

SsmsEvent::~SsmsEvent()
{
    OnClose();
}

void SsmsEvent::OnRead()
{

}

void SsmsEvent::OnWrite()
{

}

void SsmsEvent::OnClose()
{
    if (fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
        closed_ = true;
    }
}

void SsmsEvent::OnError()
{

}

void SsmsEvent::EnableReadEvent(bool enable)
{
    
}

void SsmsEvent::EnableWriteEvent(bool enable)
{
    
}

void SsmsEvent::SetEvent(int event)
{
    event_ = event;
}