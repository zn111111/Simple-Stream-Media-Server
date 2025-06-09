#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include "SsmsConnection.h"
#include "SsmsEventLoop.h"

using namespace ssms::nw;

SsmsConnection::SsmsConnection(SsmsEventLoop *loop, const SsmsNetAddressPtr &client_addr, SsmsNetAddressPtr server_addr, int fd)
: loop_(loop), SsmsEvent(fd), client_addr_(client_addr), server_addr_(server_addr)
{

}

SsmsConnection::~SsmsConnection()
{

}

void SsmsConnection::EnableReadEvent(bool enable)
{
    loop_->EnableReadEvent(shared_from_this(), enable);
}

void SsmsConnection::EnableWriteEvent(bool enable)
{
    loop_->EnableWriteEvent(shared_from_this(), enable);
}
