#pragma once

#include <vector>
#include <sys/uio.h>
#include <mutex>
#include <functional>
#include <list>
#include <unordered_map>
#include "Base/SsmsBuffer.h"
#include "Media/SsmsMediaDefine.h"
#include "SsmsNetworkDefine.h"
#include "SsmsConnection.h"

using namespace ssms::base;
using namespace ssms::media;

namespace ssms
{
    namespace media
    {
        class SsmsRtmpHandshakeContext;
    }

    namespace live
    {
        class SsmsSession;
    }
}

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoop;
        class TcpConnection : public SsmsConnection
        {
            friend class ssms::media::SsmsRtmpHandshakeContext;
            friend class ssms::live::SsmsSession;
            friend class SsmsTcpServer;
        public:
            TcpConnection(SsmsEventLoop *loop, const SsmsNetAddressPtr &client_addr, SsmsNetAddressPtr server_addr, int fd);
            ~TcpConnection();

            void SetContext(const SsmsContextPtr &context);
            void SetWriteCompleteCallback(const BusinessWriteCompleteCallback &callback);
            void SetWriteCompleteCallback(BusinessWriteCompleteCallback &&callback);
            void SetCloseCallback(const BusinessCloseCallback &callback);
            void SetCloseCallback(BusinessCloseCallback &&callback);
            //读事件
            virtual void OnRead() override;
            //写事件
            virtual void OnWrite() override;
            //关闭
            virtual void OnClose() override;
            //错误事件
            virtual void OnError() override;
            //发送单个数据包, 事件循环外调用
            void SendPkt(char *data, uint32_t len);
            //发送单个数据包, 事件循环内调用
            void SendPktInLoop(char *data, uint32_t len);
            void SendNodes(const std::list<BufferNodePtr> &iovecs);
        private:
            //发送单个数据包, 可能会被其他线程调用, 不监听，先尝试发送, 发送失败或没发完就入队
            void OnSendPkt(char *data, uint32_t len);

            std::vector<struct iovec> iovecs_;
            //读数据缓冲区
            SsmsBufferPtr buffer_;
            //写完数据执行的回调
            BusinessWriteCompleteCallback write_callback_;
            //关闭连接时执行的回调
            BusinessCloseCallback close_callback_;
            SsmsContextPtr context_;
        };
    }
}