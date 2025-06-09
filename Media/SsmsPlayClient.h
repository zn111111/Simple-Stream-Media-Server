#pragma once

#include "SsmsClient.h"
#include "Live/SsmsLiveDefine.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::live;
using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsPlayClient : public SsmsClient
        {
            friend class ssms::nw::TcpConnection;
        public:
            SsmsPlayClient(const std::string &app,
                            const std::string &stream,
                            const SsmsLiveManagmentPtr &live_manage,
                            const TcpConnectionPtr &conn,
                            SsmsRtmpMessageContextPtr context,
                            SsmsEventLoop *loop);
            ~SsmsPlayClient() = default;
            
            int Process(const SsmsPacketPtr &data, const std::string &command, double trans_id = 999999.999999) override;
            //是否是新的拉流客户端
            bool NewComming();
            //设置为旧的拉流客户端
            void SetToOld();
            void Play(const SsmsPacketPtr &pkt, bool fmt0);
        private:
            int GetStreamLengthResponse();
            int PlayResponse(double trans_id);
            void Addtask(const SsmsPacketPtr &pkt, bool fmt0);
        };
    }
}