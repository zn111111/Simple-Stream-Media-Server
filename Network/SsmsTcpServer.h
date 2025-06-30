#pragma once

#include <unordered_set>
#include "Base/NonCopyable.h"
#include "SsmsNetAddress.h"
#include "SsmsAcceptor.h"
#include "SsmsTcpConnection.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsEventLoop;
        class SsmsTcpServer : public NonCopyable
        {
        public:
            SsmsTcpServer(SsmsEventLoop *loop, const SsmsNetAddressPtr &local_addr, const SsmsLiveManagmentPtr &live_manage);
            virtual ~SsmsTcpServer();
            
            //上层业务设置接收完数据后执行的回调
            void SetReceiveCallback(const BusinessReceiveCallback &callback);
            void SetReceiveCallback(BusinessReceiveCallback &&callback);
            //上层业务设置写完数据后执行的回调
            void SetWriteCompleteCallback(const BusinessWriteCompleteCallback &callback);
            void SetWriteCompleteCallback(BusinessWriteCompleteCallback &&callback);
            //上层业务设置连接关闭后执行的回调
            void SetCloseCallback(const BusinessCloseCallback &callback);
            void SetCloseCallback(BusinessCloseCallback &&callback);
            void Start();
        private:
            void AfterAccept(SsmsEventLoop *loop, int fd, const SsmsNetAddressPtr &local, const SsmsNetAddressPtr &remote, SsmsServerProtocol protocol);    
            void AfterClose(const TcpConnectionPtr &conn);

            SsmsEventLoop *loop_ {nullptr};
            SsmsAcceptorPtr rtmp_acceptor_;
            SsmsLiveManagmentPtr live_manage_;
            std::unordered_set<TcpConnectionPtr> connections_;
            BusinessReceiveCallback recv_callback_;
            BusinessWriteCompleteCallback write_callback_;
            BusinessCloseCallback close_callback_;
        };
    }
}