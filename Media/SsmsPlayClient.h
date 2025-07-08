#pragma once

#include <atomic>
#include <vector>
#include <mutex>
#include <list>
#include "SsmsClient.h"
#include "Live/SsmsLiveDefine.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::live;
using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsStream;
        class SsmsPlayClient : public SsmsClient
        {
            friend class ssms::nw::TcpConnection;
            friend class ssms::media::SsmsStream;
        public:
            SsmsPlayClient(const std::string &app,
                            const std::string &stream_name,
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
            void Play();
            void Active();
            void DeActive();
        private:
            int GetStreamLengthResponse();
            int PlayResponse(double trans_id);
            void PostMessage(const SsmsPacketPtr &pkt, bool fmt0);

            //已经入out_packet_的包的索引
            int64_t out_packet_index_{-1};
            //已经发送的最新的视频帧的时间戳
            uint32_t out_video_timestamp_{0};
            //等待发送的包
            std::list<SsmsPacketPtr> out_packet_;
            //元数据
            SsmsPacketPtr meta_;
            //aac序列头
            SsmsPacketPtr aac_sequence_header_;
            //avc序列头
            SsmsPacketPtr avc_sequence_header_;
            //active状态表示正在发送数据, 无法再次激活, 只有发完数据关闭active状态才可以再次active
            //如果不设置active状态, 会出现推流端一直来数据, 拉流端发送不及时导致拉流端数据大量堆积的情况
            std::atomic<bool> active_;
            SsmsTimestampCorrectorPtr corrector_;
            int audio_header_version_{0};
            int video_header_version_{0};
            int meta_header_version_{0};
        };
    }
}