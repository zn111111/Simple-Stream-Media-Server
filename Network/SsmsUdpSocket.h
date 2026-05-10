#pragma once

#include <list>
#include "SsmsConnection.h"
#include "Base/SsmsBuffer.h"
#include "SsmsUdpPkt.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsUdpSocket : public SsmsConnection
        {
        public:
            SsmsUdpSocket(SsmsEventLoop *loop, int fd, SsmsNetAddressPtr server_addr, const SsmsNetAddressPtr &client_addr = nullptr);
            ~SsmsUdpSocket() = default;

            void SetMessageCallback(const UdpMessageCallback &callback);
            void SetMessageCallback(UdpMessageCallback &&callback);
            void SetWriteCompleteCallback(const UdpWriteCompleteCallback &callback);
            void SetWriteCompleteCallback(UdpWriteCompleteCallback &&callback);
            void SetCloseCallback(const UdpCloseCallback &callback);
            void SetCloseCallback(UdpCloseCallback &&callback);
            //读事件
            void OnRead() override;
            //写事件
            void OnWrite() override;
            //关闭
            void OnClose() override;
            //错误事件
            void OnError() override;
            void SendPacket(const SsmsUdpPktPtr &pkt);
        private:
            //处理接收数据的回调
            UdpMessageCallback message_callback_;
            //写完数据执行的回调
            UdpWriteCompleteCallback write_callback_;
            //关闭连接时执行的回调
            UdpCloseCallback close_callback_;
            //读数据缓冲区
            SsmsUdpPktPtr buffer_;
            //待发送包
            std::list<SsmsUdpPktPtr> pkt_list_;
        };
    }
}