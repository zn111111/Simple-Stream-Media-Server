#include "SsmsPublishClient.h"
#include "SsmsMediaDefine.h"
#include "Base/SsmsLogStream.h"
#include "Live/SsmsLiveManagment.h"
#include "SsmsAmf0Message.h"
#include "SsmsPacket.h"
#include "Network/SsmsTcpConnection.h"
#include "Live/SsmsSession.h"
#include "Base/SsmsUtils.h"
#include "Network/SsmsEventLoop.h"
#include "SsmsStream.h"

using namespace ssms::media;

SsmsPublishClient::SsmsPublishClient(const std::string &app,
                const std::string &stream,
                const SsmsLiveManagmentPtr &live_manage,
                const TcpConnectionPtr &conn,
                SsmsRtmpMessageContextPtr context,
                SsmsEventLoop *loop)
: SsmsClient(app, stream, live_manage, conn, context, loop)
{

}

int SsmsPublishClient::Process(const SsmsPacketPtr &data, const std::string &command, double trans_id)
{
    int ret = 0;
    RtmpMessageHeaderPtr header = data->Ext<struct RtmpMessageHeader>();
    if (!header)
    {
        LOG_ERROR << "rtmp data error, does not have rtmp header info";
        return -1;
    }

    int offset = 0;
    switch (header->message_type_id)
    {
        case RtmpMessageAudio:
        case RtmpMessageVideo:
            ret = ProcessAudioVideo(data);
            break;
        case RtmpMessageAMF3MetaData:
            offset += 1;
        case RtmpMessageAMF0MetaData:
            ret = ParseDataMessage(data, offset);
            break;
        case RtmpMessageAMF3Command:
        case RtmpMessageAMF0Command:
            if ("releaseStream" == command)
            {
                if (SsmsUtils::Compare(trans_id, 999999.999999))
                {
                    LOG_DEBUG << "transaction id error";
                    ret = -1;
                    break;
                }
                ret = ReleaseStreamResponse(trans_id);
            }
            else if ("FCPublish" == command)
            {
                if (SsmsUtils::Compare(trans_id, 999999.999999))
                {
                    LOG_DEBUG << "transaction id error";
                    ret = -1;
                    break;
                }
                ret = FCPublishResponse(trans_id);
            }
            else if ("publish" == command)
            {
                if (SsmsUtils::Compare(trans_id, 999999.999999))
                {
                    LOG_DEBUG << "transaction id error";
                    ret = -1;
                    break;
                }
                ret = PublishResponse(trans_id);
            }
            else if ("FCUnpublish" == command)
            {
                LOG_DEBUG << "stream end";
                ret = 2;
            }
            else
            {
                LOG_DEBUG << "unsupported rtmp command message";
                ret = -1;
            }
            break;
    }

    return ret;
}

int SsmsPublishClient::ReleaseStreamResponse(double trans_id)
{
    //释放可能存在的未释放的流
    live_manage_->DeleteSession(app_name_ + "/" + stream_name_);
    std::shared_ptr<SsmsAmf0String> command = std::make_shared<SsmsAmf0String>();
    command->SetValue("_result");
    std::shared_ptr<SsmsAmf0Number> transaction_id = std::make_shared<SsmsAmf0Number>();
    transaction_id->SetValue(trans_id);
    std::shared_ptr<SsmsAmf0NULL> null = std::make_shared<SsmsAmf0NULL>();
    std::shared_ptr<SsmsAmf0Undefined> undefined = std::make_shared<SsmsAmf0Undefined>();
    int payload_size = command->EncodeSize() + transaction_id->EncodeSize() + null->EncodeSize() + undefined->EncodeSize();
    SsmsPacketPtr pkt = std::make_shared<SsmsPacket>(payload_size);
    int offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += transaction_id->Encode(pkt->data + offset);
    offset += null->Encode(pkt->data + offset);
    offset += undefined->Encode(pkt->data + offset);
    RtmpMessageHeaderPtr header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = payload_size;
    header->message_type_id = 20;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    (std::move(pkt), true);
    LOG_DEBUG << "send rtmp releaseStream response";
    return 0;
}

int SsmsPublishClient::FCPublishResponse(double trans_id)
{
    std::shared_ptr<SsmsAmf0String> command = std::make_shared<SsmsAmf0String>();
    command->SetValue("_result");
    std::shared_ptr<SsmsAmf0Number> transaction_id = std::make_shared<SsmsAmf0Number>();
    transaction_id->SetValue(trans_id);
    std::shared_ptr<SsmsAmf0NULL> null = std::make_shared<SsmsAmf0NULL>();
    std::shared_ptr<SsmsAmf0Undefined> undefined = std::make_shared<SsmsAmf0Undefined>();
    int payload_size = command->EncodeSize() + transaction_id->EncodeSize() + null->EncodeSize() + undefined->EncodeSize();
    SsmsPacketPtr pkt = std::make_shared<SsmsPacket>(payload_size);
    int offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += transaction_id->Encode(pkt->data + offset);
    offset += null->Encode(pkt->data + offset);
    offset += undefined->Encode(pkt->data + offset);
    RtmpMessageHeaderPtr header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = payload_size;
    header->message_type_id = 20;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp FCPublish response";
    return 0;
}

int SsmsPublishClient::PublishResponse(double trans_id)
{
    sess_ = live_manage_->CreateSession(app_name_ + "/" + stream_name_);
    sess_->SetProducer(std::dynamic_pointer_cast<SsmsPublishClient>(shared_from_this()));

    std::shared_ptr<SsmsAmf0String> command = std::make_shared<SsmsAmf0String>();
    command->SetValue("onFCPublish");
    std::shared_ptr<SsmsAmf0Number> transaction_id = std::make_shared<SsmsAmf0Number>();
    transaction_id->SetValue(trans_id);
    std::shared_ptr<SsmsAmf0NULL> null = std::make_shared<SsmsAmf0NULL>();
    SsmsAmf0TypePtr object1 = std::make_shared<SsmsAmf0Object>();
    //code
    SsmsAmf0TypePtr propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("NetStream.Publish.Start");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("code", propertie);
    //description
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("Started publishing stream.");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("description", propertie);

    uint32_t pkt_len = command->EncodeSize() + transaction_id->EncodeSize() + null->EncodeSize() + object1->EncodeSize();
    SsmsPacketPtr pkt = std::make_shared<SsmsPacket>(pkt_len);
    int offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += transaction_id->Encode(pkt->data + offset);
    offset += null->Encode(pkt->data + offset);
    offset += object1->Encode(pkt->data + offset);
    RtmpMessageHeaderPtr header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = pkt_len;
    header->message_type_id = 20;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send onFCPublish";

    command = std::make_shared<SsmsAmf0String>();
    command->SetValue("onStatus");
    transaction_id = std::make_shared<SsmsAmf0Number>();
    transaction_id->SetValue(trans_id);
    null = std::make_shared<SsmsAmf0NULL>();
    object1 = std::make_shared<SsmsAmf0Object>();
    //level
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("status");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("level", propertie);
    //code
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("NetStream.Publish.Start");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("code", propertie);
    //description
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("Started publishing stream.");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("description", propertie);
    //clientid
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("ASAICiss");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("clientid", propertie);

    pkt_len = command->EncodeSize() + transaction_id->EncodeSize() + null->EncodeSize() + object1->EncodeSize();
    pkt = std::make_shared<SsmsPacket>(pkt_len);
    offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += transaction_id->Encode(pkt->data + offset);
    offset += null->Encode(pkt->data + offset);
    offset += object1->Encode(pkt->data + offset);
    header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = pkt_len;
    header->message_type_id = 20;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send publish response";

    return 0;   
}

int SsmsPublishClient::ProcessAudioVideo(const SsmsPacketPtr &data)
{
    //avc序列尾标志
    int ret = 0;
    if (data->payload_size_ > 2 && FlvVideoCodecAVC == (*data->data & 0x0F) && 2 == *(data->data + 1))
    {
        LOG_DEBUG << "AVC end of sequence";
        return ret;
    }

    if (data->IsAudio() && !data->IsAudioSequenceHeader())
    {
        data->Ext<RtmpMessageHeader>()->timestamp = pre_audio_timestamp;
        pre_audio_timestamp += a_frame_interval;
    }
    else if (data->IsVideo() && !data->IsVideoSequenceHeader())
    {
        data->Ext<RtmpMessageHeader>()->timestamp = pre_video_timestamp;
        pre_video_timestamp += v_frame_interval;
    }
    SsmsStreamPtr stream = sess_->Stream();
    stream->Push(data);
    sess_->ActiveAll();

    return ret;
}

int SsmsPublishClient::ParseDataMessage(const SsmsPacketPtr &data, uint32_t offset)
{
    RtmpMessageHeader header = *(data->Ext<RtmpMessageHeader>());
    char *p = data->data;
    if (data->payload_size_ < offset)
    {
        LOG_DEBUG << "Data message format error";
        return -1;
    }

    uint16_t len = 0;
    bool is_meta_data = false;
    while (offset + 3 <= data->payload_size_ && Amf0String == *(uint8_t *)(p + offset))
    {
        offset++;
        len = ::ntohs(*(uint16_t *)(p + offset));
        offset += 2;
        if (offset + len > data->payload_size_)
        {
            LOG_DEBUG << "Data message format error";
            return -1;    
        }

        if (len == META_DATA_FLAG_SIZE && !memcmp(p + offset, META_DATA_FLAG, META_DATA_FLAG_SIZE))
        {
            offset += len;
            is_meta_data = true;
            break;
        }
        else
        {
            offset += len;
        }
    }
    if (!is_meta_data)
    {
        LOG_DEBUG << "Data message format error, is not meta data";
        return -1;
    }

    int marker = 0;
    if (offset + 1 > data->payload_size_ || (Amf0EcmaArray != (marker = (uint8_t)*(p + offset)) && Amf0Object != marker))
    {
        LOG_DEBUG << "Data message format error, marker is " << marker;
        return -1;
    }
    offset++;
    std::shared_ptr<ssms::media::SsmsAmf0Type> amf_data;
    if (Amf0EcmaArray == marker)
    {
         amf_data = std::make_shared<SsmsAmf0EcmaArray>();
    }
    else
    {
        amf_data = std::make_shared<SsmsAmf0Object>();
    }
    if (amf_data->Parse(data->data + offset, data->payload_size_ - offset) < 0)
    {
        LOG_DEBUG << "SetDataFrame data format error";
        return -1;
    }
    uint16_t frame_rate = 0, sample_rate = 0;
    std::string s_frame_rate, s_sample_rate;
    if (!(s_frame_rate = amf_data->GetProperty("framerate")).empty() && (frame_rate = atoi(s_frame_rate.c_str())) > 0)
    {
        v_frame_interval = 1000 / frame_rate;
    }
    if (!(s_sample_rate = amf_data->GetProperty("audiosamplerate")).empty() && (sample_rate = atoi(s_sample_rate.c_str())) > 0)
    {
        //支支持aac, 1024是aac的每帧采样点的数量
        a_frame_interval = 1000 * 1024 / sample_rate;
    }

    SsmsStreamPtr stream = sess_->Stream();
    stream->Push(data);

    return 0;
}

void SsmsPublishClient::PostMessage(const SsmsPacketPtr &pkt, bool fmt0)
{
    loop_->AddTask([this, pkt] () {
        context_->BuildChunk(pkt, true);
        context_->SendNodes();
    });
}