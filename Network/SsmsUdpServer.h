#pragma once

#include "SsmsNetworkDefine.h"
#include "Base/NonCopyable.h"
#include "SsmsUdpSocket.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoop;
        class SsmsUdpServer : public SsmsUdpSocket
        {
        public:
            SsmsUdpServer(SsmsEventLoop *loop,
                        const SsmsLiveManagmentPtr &live_manage,
                        SsmsNetAddressPtr server_addr,
                        SsmsWebrtcServerPtr rtc_server);
            ~SsmsUdpServer() = default;
            
            void Start();
            void SetMessageCallback(const UdpMessageCallback &callback);
            void SetMessageCallback(UdpMessageCallback &&callback);
            void SetWriteCompleteCallback(const UdpWriteCompleteCallback &callback);
            void SetWriteCompleteCallback(UdpWriteCompleteCallback &&callback);
            //处理读事件
            void OnRead() override;
            //处理写事件
            void OnWrite() override;
            //处理关闭事件
            void OnClose() override;
            //处理错误事件
            void OnError() override;
        private:
            //处理接收数据的回调
            UdpMessageCallback message_callback_;
            //写完数据执行的回调
            UdpWriteCompleteCallback write_callback_;
            SsmsEventLoop *loop_{nullptr};
            SsmsLiveManagmentPtr live_manage_;
            SsmsWebrtcServerPtr rtc_server_;
        };
    }
}