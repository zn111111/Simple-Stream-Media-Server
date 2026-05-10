#pragma once

#include <memory>
#include <list>
#include <unordered_map>
#include "SsmsContext.h"
#include "SsmsPacket.h"
#include "Network/SsmsNetworkDefine.h"
#include "Base/SsmsHttpHandler.h"
#include "SsmsSdp.h"

using namespace ssms::nw;
using namespace ssms::media::base;

namespace ssms
{
    namespace media
    {
        //解析HTTP协议
        class SsmsHttpMessageContext : public SsmsContext
        {
            friend class ssms::nw::TcpConnection;
        public:
            SsmsHttpMessageContext(SsmsEventLoop *loop,
                                TcpConnectionPtr conn,
                                SsmsLiveManagmentPtr live_manage,
                                SsmsWebrtcServerPtr rtc_server);
            ~SsmsHttpMessageContext() = default;

            //返回值小于0表示解析出错, 等于0表示解析成功, 1表示数据不够, 2表示流结束
            int Parse(const SsmsBufferPtr &data) override;
        private:
            void ClearSendCompleteData() override;

            SsmsEventLoop *loop_{nullptr};
            std::weak_ptr<TcpConnection> conn_;
            //正在发送的源数据队列, 发送完由回调删除
            std::list<SsmsPacketPtr> sending_pkts_;
            SsmsWebrtcServerPtr rtc_server_;
        };
    }
}