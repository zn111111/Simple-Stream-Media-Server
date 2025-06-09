#pragma once

#include <memory>

namespace ssms
{
    namespace nw
    {
        class SsmsEvent : public std::enable_shared_from_this<SsmsEvent>
        {
            friend class SsmsEventLoop;
        public:
            SsmsEvent(int fd);
            virtual ~SsmsEvent();
            virtual void OnRead();
            virtual void OnWrite();
            virtual void OnClose();
            virtual void OnError();
            virtual void EnableReadEvent(bool enable);
            virtual void EnableWriteEvent(bool enable);
            void SetEvent(int event);

            int Fd() const
            {
                return fd_;
            }
        protected:
            int fd_{-1};
            //事件循环中可能会有多个相同的fd, 因此需要确保连接是否关闭
            bool closed_{false};
            //EPOLLIN等flag
            int event_{0};
        };
    }
}