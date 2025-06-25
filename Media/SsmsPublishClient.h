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
        class SsmsPublishClient : public SsmsClient
        {
        public:
            SsmsPublishClient(const std::string &app,
                            const std::string &stream,
                            const SsmsLiveManagmentPtr &live_manage,
                            const TcpConnectionPtr &conn,
                            SsmsRtmpMessageContextPtr context,
                            SsmsEventLoop *loop);
            ~SsmsPublishClient() = default;

            int Process(const SsmsPacketPtr &data, const std::string &command, double trans_id = 999999.999999) override;
        private:
            int ReleaseStreamResponse(double trans_id);
            int FCPublishResponse(double trans_id);
            int PublishResponse(double trans_id);
            int ProcessAudioVideo(const SsmsPacketPtr &data);
            int ParseSetDataFrame(const SsmsPacketPtr &data, uint32_t offset);
            void PostMessage(const SsmsPacketPtr &pkt, bool fmt0);

            //上次音频的时间戳
            uint32_t pre_audio_timestamp{0};
            //上次视频的时间戳
            uint32_t pre_video_timestamp{0};

            //音频帧间隔, 单位毫秒
            uint16_t a_frame_interval{0};
            //视频帧间隔, 单位毫秒
            uint16_t v_frame_interval{0};
        };
    }
}