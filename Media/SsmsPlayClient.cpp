#include "SsmsPlayClient.h"
#include "SsmsMediaDefine.h"
#include "Base/SsmsUtils.h"
#include "SsmsPacket.h"
#include "SsmsLogStream.h"
#include "SsmsAmf0Message.h"
#include "Live/SsmsLiveManagment.h"
#include "Live/SsmsSession.h"
#include "Network/SsmsTcpConnection.h"
#include "Network/SsmsEventLoop.h"
#include "SsmsStream.h"
#include "SsmsGopManagment.h"
#include "SsmsINIReader.h"
#include "SsmsTimestampCorrector.h"

using namespace ssms::media;
using namespace ssms::base;

SsmsPlayClient::SsmsPlayClient(const std::string &app,
                const std::string &stream_name,
                const SsmsLiveManagmentPtr &live_manage,
                const TcpConnectionPtr &conn,
                SsmsRtmpMessageContextPtr context,
                SsmsEventLoop *loop)
: SsmsClient(app, stream_name, live_manage, conn, context, loop)
{
    active_.store(false);
    corrector_ = std::make_shared<SsmsTimestampCorrector>();
}

int SsmsPlayClient::Process(const SsmsPacketPtr &data, const std::string &command, double trans_id)
{
    int ret = 0;
    RtmpMessageHeaderPtr header = data->Ext<struct RtmpMessageHeader>();
    if (!header)
    {
        LOG_ERROR << "rtmp data error, does not have rtmp header info";
        return -1;
    }

    switch (header->message_type_id)
    {
        case RtmpMessageAudio:
        case RtmpMessageVideo:
            break;
        case RtmpMessageAMF3MetaData:
        case RtmpMessageAMF0MetaData:
            break;
        case RtmpMessageAMF3Command:
        case RtmpMessageAMF0Command:
            if ("getStreamLength" == command)
            {
                ret = GetStreamLengthResponse();
            }
            else if ("play" == command)
            {
                if (SsmsUtils::Compare(trans_id, 999999.999999))
                {
                    LOG_DEBUG << "transaction id error";
                    ret = -1;
                    break;
                }
                ret = PlayResponse(trans_id);
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

bool SsmsPlayClient::NewComming()
{
    return conn_->NewConnection();
}

void SsmsPlayClient::SetToOld()
{
    conn_->SetToOldConnection();
}

void SsmsPlayClient::Play()
{
    SsmsStreamPtr stream = sess_->Stream();

    if (out_packet_.empty())
    {
        stream->Pop(std::dynamic_pointer_cast<SsmsPlayClient>(shared_from_this()));
    }
    if (meta_)
    {
        context_->BuildChunk(meta_, true);
        meta_.reset();
    }
    if (aac_sequence_header_)
    {
        context_->BuildChunk(aac_sequence_header_, true);
        aac_sequence_header_.reset();
    }
    if (avc_sequence_header_)
    {
        context_->BuildChunk(avc_sequence_header_, true);
        avc_sequence_header_.reset();
    }

    for (auto it = out_packet_.begin(); it != out_packet_.end();)
    {
        uint32_t corrected_timestamp = corrector_->CorrectTimestamp(*it);
        (*it)->SetTimestamp(corrected_timestamp);
        if (context_->BuildChunk(*it, true))
        {
            //out_packet_里不会有头部, 这里就不加头部的判断了
            if ((*it)->IsVideo())
            {
                out_video_timestamp_ = corrected_timestamp;
            }
            it = out_packet_.erase(it);
        }
        else
        {
            break;
        }
    }
    context_->SendNodes();
}

void SsmsPlayClient::Active()
{
    if (!active_.load())
    {
        loop_->AddTask([this] () {
            Play();
        });
        active_.store(true);
    }
}

void SsmsPlayClient::DeActive()
{
    active_.store(false);
}

int SsmsPlayClient::GetStreamLengthResponse()
{
    SsmsPacketPtr pkt = std::make_shared<SsmsPacket>(6);
    RtmpMessageHeaderPtr header = std::make_shared<RtmpMessageHeader>();
    header->csid = 2;
    header->timestamp = 0;
    header->payload_len = 6;
    header->message_type_id = 4;
    header->stream_id = 0;
    SsmsUtils::Write2BytesBe(pkt->data, 0);
    SsmsUtils::Write4BytesBe(pkt->data + 2, 1);
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    SsmsUtils::Write4BytesBe(pkt->data, 128);
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp StreamBegin";
    return 0;
}

int SsmsPlayClient::PlayResponse(double trans_id)
{
    if (!live_manage_->Exist(app_name_ + "/" + stream_name_))
    {
        LOG_DEBUG << "stream name " << app_name_ << "/" << stream_name_ << " is not exist";
        return -1;
    }

    sess_ = live_manage_->GetSession(app_name_ + "/" + stream_name_);
    sess_->AddConsumer(std::dynamic_pointer_cast<SsmsPlayClient>(shared_from_this()));

    std::shared_ptr<SsmsAmf0String> command = std::make_shared<SsmsAmf0String>();
    command->SetValue("onStatus");
    std::shared_ptr<SsmsAmf0Number> transaction_id = std::make_shared<SsmsAmf0Number>();
    transaction_id->SetValue(trans_id);
    std::shared_ptr<SsmsAmf0NULL> null = std::make_shared<SsmsAmf0NULL>();
    SsmsAmf0TypePtr object1 = std::make_shared<SsmsAmf0Object>();
    //level
    SsmsAmf0TypePtr propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("status");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("level", propertie);
    //code
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("NetStream.Play.Reset");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("code", propertie);
    //description
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("Playing and resetting stream.");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("description", propertie);
    //details
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("stream");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("details", propertie);
    //clientid
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("ASAICiss");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("clientid", propertie);

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
    header->stream_id = 1;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp onStatus";

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
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("NetStream.Play.Start");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("code", propertie);
    //description
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("Started playing stream.");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("description", propertie);
    //details
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("stream");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("details", propertie);
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
    header->stream_id = 1;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp onStatus";

    command = std::make_shared<SsmsAmf0String>();
    command->SetValue("|RtmpSampleAccess");
    std::shared_ptr<SsmsAmf0Bool> b1 = std::make_shared<SsmsAmf0Bool>();
    b1->SetValue(true);
    std::shared_ptr<SsmsAmf0Bool> b2 = std::make_shared<SsmsAmf0Bool>();
    b2->SetValue(true);

    pkt_len = command->EncodeSize() + b1->EncodeSize() + b2->EncodeSize();
    pkt = std::make_shared<SsmsPacket>(pkt_len);
    offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += b1->Encode(pkt->data + offset);
    offset += b2->Encode(pkt->data + offset);
    header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = pkt_len;
    header->message_type_id = 18;
    header->stream_id = 1;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp |RtmpSampleAccess";

    command = std::make_shared<SsmsAmf0String>();
    command->SetValue("onStatus");
    object1 = std::make_shared<SsmsAmf0Object>();
    //code
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("NetStream.Data.Start");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("code", propertie);

    pkt_len = command->EncodeSize() + object1->EncodeSize();
    pkt = std::make_shared<SsmsPacket>(pkt_len);
    offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += object1->Encode(pkt->data + offset);
    header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = pkt_len;
    header->message_type_id = 20;
    header->stream_id = 1;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp onStatus";

    return 0;
}

void SsmsPlayClient::PostMessage(const SsmsPacketPtr &pkt, bool fmt0)
{
    loop_->AddTask([this, pkt] () {
        context_->BuildChunk(pkt, true);
        context_->SendNodes();
    });
}