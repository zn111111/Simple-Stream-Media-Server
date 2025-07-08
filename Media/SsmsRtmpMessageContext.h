#pragma once

#include <memory>
#include <list>
#include <unordered_map>
#include "SsmsContext.h"
#include "SsmsPacket.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsRtmpHandshakeContext;
        //解析RTMP协议握手、命令消息以及控制消息等数据
        class SsmsRtmpMessageContext : public SsmsContext
        {
            friend class ssms::media::SsmsRtmpHandshakeContext;
            friend class ssms::nw::TcpConnection;
        public:
            SsmsRtmpMessageContext(SsmsEventLoop *loop, const TcpConnectionPtr &conn, SsmsLiveManagmentPtr live_manage);
            ~SsmsRtmpMessageContext();

            //返回值小于0表示解析出错, 等于0表示解析成功, 1表示数据不够, 2表示流结束
            int Parse(const SsmsBufferPtr &data) override;
            //将pkt拆分成一个个chunk, 存在sending_nodes_中
            //控制消息第一个chunk必须是fmt0, 但是音视频消息可以根据实际情况选择是否使用fmt0
            bool BuildChunk(const SsmsPacketPtr &pkt, bool fmt0);
            void SendNodes();
        private:
            int ParseMessage(const SsmsBufferPtr &data);
            int ParseAssembledMessage(const SsmsPacketPtr &data);
            int ParseCommandMessage(const SsmsPacketPtr &data, uint32_t offset);
            int ConnectResponse(double transaction_id, const std::string &app);
            int ParseSetChunkSize(const SsmsPacketPtr &data);
            int ParseUserControlMessage(const SsmsPacketPtr &data);
            int ParseWindowAcknowledgementSize(const SsmsPacketPtr &data);
            int ProcessAudioVideo(const SsmsPacketPtr &data);
            int ParseDataMessage(const SsmsPacketPtr &data);
            int CreateStreamResponse(double trans_id);
            void ClearSendCompleteData() override;
            //解析Amf编码的数据, 如果有匹配的command, 默认会解析流名称, 否则out_data是SsmsAmf0Object或者SsmsAmf0EcmaArray
            int ParseAmfData(const SsmsPacketPtr &data, uint32_t offset, const std::string &command, SsmsAmf0TypePtr &out_data);
            void PostMessage(const SsmsPacketPtr &pkt, bool fmt0);

            SsmsEventLoop *loop_{nullptr};
            SsmsRtmpHandshakeContextPtr handshake_;
            std::weak_ptr<TcpConnection> conn_;
            RtmpMessageState state_{RtmpMessageHandshake};
            //csid : RtmpMessageHeader, 上一个接收的包的头部
            std::unordered_map<int, RtmpMessageHeaderPtr> prev_recv_headers_;
            //上一个发送的包的头部
            std::unordered_map<int, RtmpMessageHeaderPtr> prev_send_headers_;
            //未接收完成的数据包
            std::unordered_map<int, SsmsPacketPtr> packets_;
            //服务器的发送chunk大小
            uint32_t s_chunk_size_{128};
            //客户端的发送chunk大小
            uint32_t r_chunk_size_{128};
            //客户端的确认窗口大小
            uint32_t client_window_ack_size_{0};
            //正在发送的数据的头部, 只用于存储待发送数据的rtmp头部
            char sending_[10240]{0};
            //sending_当前位置索引
            uint32_t sending_curr_{0};
            //待发送队列
            std::list<SsmsPacketPtr> waiting_to_send_;
            //正在发送的源数据队列, 发送完由回调删除
            std::list<SsmsPacketPtr> sending_pkts_;
            //仅用于调用BuildChunk时临时存储切分好的一个个chunk的BufferNodePtr
            std::list<BufferNodePtr> sending_nodes_;
            //csid : RtmpMessageHeader, 上一个发送的包的时间戳差值
            std::unordered_map<int, uint32_t> prev_timestamp_deltas_;
        };
    }
}