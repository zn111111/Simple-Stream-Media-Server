#pragma once

#include <functional>
#include "SsmsNetworkDefine.h"
#include "SsmsEvent.h"

using namespace ssms::live;

namespace ssms
{
    namespace nw
    {
        class SsmsAcceptor : public SsmsEvent
        {
        public:
            SsmsAcceptor(SsmsEventLoop *loop, const SsmsNetAddressPtr &addr, SsmsServerProtocol protocol);
            ~SsmsAcceptor();

            void SetAcceptCallback(const AcceptCallback &cb);
            void SetAcceptCallback(AcceptCallback &&cb);
            void StartListen();
            //处理读事件
            void OnRead() override;
            //处理关闭事件
            void OnClose() override;
            //处理错误事件
            void OnError() override;
        private:
            void OnAccept();

            SsmsEventLoop *loop_{nullptr};
            //监听的地址
            SsmsNetAddressPtr addr_;
            //接受完连接后执行的回调, 由上业务传递
            AcceptCallback cb_;
            SsmsServerProtocol protocol_{SsmsServerProtocolDefault};
        };
    }
}