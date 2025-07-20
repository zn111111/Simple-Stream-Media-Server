#pragma once

#include "SsmsNetworkDefine.h"
#include "SsmsEvent.h"

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoop;
        class SsmsConnection : public SsmsEvent
        {
        public:
            SsmsConnection(SsmsEventLoop *loop, const SsmsNetAddressPtr &client_addr, SsmsNetAddressPtr server_addr, int fd);
            virtual ~SsmsConnection();

            //开启或关闭读事件
            virtual void EnableReadEvent(bool enable) override;
            //开启或关闭写事件
            virtual void EnableWriteEvent(bool enable) override;
            bool Alive() const;
            void Reset();
        protected:
            SsmsEventLoop *loop_{nullptr};
            SsmsNetAddressPtr client_addr_;
            SsmsNetAddressPtr server_addr_;
            //连接是否活跃, 有数据交换表示活跃
            bool connection_alive_{false};
        };
    }
}