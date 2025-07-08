#include <arpa/inet.h>
#include "SsmsRtmpMessageContext.h"
#include "Base/SsmsLogStream.h"
#include "SsmsPacket.h"
#include "SsmsRtmpHandshakeContext.h"
#include "SsmsAmf0Message.h"
#include "Live/SsmsLiveManagment.h"
#include "Base/SsmsUtils.h"
#include "Live/SsmsSession.h"
#include "SsmsPublishClient.h"
#include "SsmsPlayClient.h"
#include "Network/SsmsEventLoop.h"

using namespace ssms::media;
using namespace ssms::live;

SsmsRtmpMessageContext::SsmsRtmpMessageContext(SsmsEventLoop *loop, const TcpConnectionPtr &conn, SsmsLiveManagmentPtr live_manage)
: loop_(loop)
, handshake_(std::make_shared<SsmsRtmpHandshakeContext>(conn))
, conn_(conn)
{
    SsmsContext::live_manage_ = live_manage;
}

SsmsRtmpMessageContext::~SsmsRtmpMessageContext()
{

}

int SsmsRtmpMessageContext::Parse(const SsmsBufferPtr &data)
{
    int ret = 0;
    while (data->readableBytes() > 0)
    {
        switch (state_)
        {
            case RtmpMessageHandshake:
                ret = handshake_->Parse(data);
                break;
            case RtmpMessageControl:
                ret = ParseMessage(data);
                break;
            default:
                LOG_ERROR << "RTMP protocol state error";
                return -1;
        }
        if (0 != ret)
        {
            break;
        }
    }

    return ret;
}

void SsmsRtmpMessageContext::ClearSendCompleteData()
{
    sending_pkts_.clear();
    sending_nodes_.clear();
    sending_curr_ = 0;
    SsmsPlayClientPtr player = std::dynamic_pointer_cast<SsmsPlayClient>(client_);
    if (player)
    {
        sess_->DeActive(player);
    }
}

int SsmsRtmpMessageContext::ParseAmfData(const SsmsPacketPtr &data, uint32_t offset, const std::string &command, SsmsAmf0TypePtr &out_data)
{
    char *p = data->data;
    uint32_t len = data->payload_size_;
    bool stream_name_parsed = false;
    int ret = 0;
    while (offset < data->payload_size_)
    {
        uint8_t type = *(p + offset++);
        switch (type)
        {
            case Amf0Number:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Number>();
                ret = amf0_data->Parse(p + offset, len - offset);
                if (ret < 0)
                {
                    LOG_DEBUG << "parse Amf0Number error";
                    return ret;
                }
                offset += ret;
            }
                break;
            case Amf0Bool:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0Bool>();
                ret = amf0_data->Parse(p + offset, len - offset);
                if (ret < 0)
                {
                    LOG_DEBUG << "parse Amf0Bool error";
                    return ret;
                }
                offset += ret;
            }
                break;
            case Amf0String:
            {
                SsmsAmf0TypePtr amf0_data = std::make_shared<SsmsAmf0String>();
                ret = amf0_data->Parse(p + offset, len - offset);
                if (ret < 0)
                {
                    LOG_DEBUG << "parse Amf0String error";
                    return ret;
                }
                offset += ret;
                if (("releaseStream" == command || "FCPublish" == command) && stream_.empty())
                {
                    stream_ = amf0_data->Value();
                }
                else if ("publish" == command && !stream_name_parsed && stream_.empty())
                {
                    stream_ = amf0_data->Value();
                    stream_name_parsed = true;
                }
                else if (("play" == command || "getStreamLength" == command) && stream_.empty())
                {
                    stream_ = amf0_data->Value();
                }
            }
                break;
            case Amf0Object:
            {
                std::shared_ptr<ssms::media::SsmsAmf0Object> s_obj = std::make_shared<SsmsAmf0Object>();
                ret = s_obj->Parse(p + offset, len - offset);
                if (ret < 0)
                {
                    LOG_DEBUG << "parse Amf0Object error";
                    return ret;
                }
                offset += ret;
                if ("connect" == command)
                {
                    app_ = s_obj->GetProperty("app");
                }
                out_data = std::move(s_obj);
            }
                break;
            case Amf0Null:
                break;
            case Amf0Undefined:
                break;
            case Amf0EcmaArray:
            {
                std::shared_ptr<ssms::media::SsmsAmf0EcmaArray> s_ecma = std::make_shared<SsmsAmf0EcmaArray>();
                ret = s_ecma->Parse(p + offset, len - offset);
                if (ret < 0)
                {
                    LOG_DEBUG << "parse Amf0EcmaArray error";
                    return ret;
                }
                offset += ret;
                out_data = std::move(s_ecma);
            }
                break;
            default:
                LOG_DEBUG << "unsupported amf type " << type;
                ret = -1;
                return ret;
        }
    }

    return ret;
}

void SsmsRtmpMessageContext::PostMessage(const SsmsPacketPtr &pkt, bool fmt0)
{
    loop_->AddTask([this, pkt] () {
        BuildChunk(pkt, true);
        SendNodes();
    });
}

int SsmsRtmpMessageContext::ParseMessage(const SsmsBufferPtr &data)
{
    if (data->readableBytes() < 1)
    {
        return 1;
    }

    //解析basic header
    const char *p_data = data->peek();
    uint32_t offset = 0;
    uint8_t basic_header = *(p_data + offset++);
    uint8_t fmt = (basic_header >> 6);
    uint8_t csid = (basic_header & 0x3F);
    if (0 == csid)
    {
        if (data->readableBytes() < 2)
        {
            return 1;
        }

        csid = 64 + (uint8_t)*(p_data + offset++);
    }
    else if (1 == csid)
    {
        if (data->readableBytes() < 3)
        {
            return 1;
        }

        csid = 64 + (uint8_t)*(p_data + offset++) + (uint8_t)*(p_data + offset++) * 256;
    }

    //解析message header
    std::shared_ptr<struct RtmpMessageHeader> header;
    if (prev_recv_headers_.find(csid) == prev_recv_headers_.end())
    {
        header = std::make_shared<struct RtmpMessageHeader>();
        prev_recv_headers_[csid] = header;
    }
    else
    {
        header = prev_recv_headers_[csid];
    }
    uint32_t timestamp = 0, payload_len = 0, stream_id = 0;
    uint8_t message_type_id = 0;
    switch (fmt)
    {
        case 0:
            if (data->readableBytes() < offset + 11)
            {
                return 1;
            }
            memcpy((uint8_t *)&timestamp + 1, p_data + offset, 3);
            timestamp = ::ntohl(timestamp);
            offset += 3;
            memcpy((uint8_t *)&payload_len + 1, p_data + offset, 3);
            payload_len = ::ntohl(payload_len);
            offset += 3;
            message_type_id = *(p_data + offset++);
            memcpy(&stream_id, p_data + offset, 4);
            offset += 4;
            break;
        case 1:
            if (data->readableBytes() < offset + 7)
            {
                return 1;
            }

            memcpy((uint8_t *)&timestamp + 1, p_data + offset, 3);
            timestamp = ::ntohl(timestamp);
            offset += 3;
            memcpy((uint8_t *)&payload_len + 1, p_data + offset, 3);
            payload_len = ::ntohl(payload_len);
            offset += 3;
            message_type_id = *(p_data + offset++);
            stream_id = header->stream_id;
            break;
        case 2:
            if (data->readableBytes() < offset + 3)
            {
                return 1;
            }

            memcpy((uint8_t *)&timestamp + 1, p_data + offset, 3);
            timestamp = ::ntohl(timestamp);
            offset += 3;
            payload_len = header->payload_len;
            message_type_id = header->message_type_id;
            stream_id = header->stream_id;
            break;
        case 3:
            timestamp = header->timestamp_delta;
            payload_len = header->payload_len;
            message_type_id = header->message_type_id;
            stream_id = header->stream_id;
            break;
        default:
            LOG_ERROR << "rtmp basic header data error";
            return -1;
    }
    if (timestamp == 0xFFFFFF)
    {
        if (data->readableBytes() < offset + 4)
        {
            return 1;
        }

        timestamp = ::ntohl(*(uint32_t *)(p_data + offset));
        offset += 4;
    }

    header->csid = (RtmpMessageAudio == message_type_id ? 8 : (RtmpMessageVideo == message_type_id ? 9
                    : ((RtmpMessageAMF3MetaData == message_type_id || RtmpMessageAMF0MetaData == message_type_id) ? 
                    6 : csid)));
    //相对时间戳(扩展时间戳也是相对时间戳)
    if (0 != fmt)
    {
        header->timestamp_delta = timestamp;
        header->timestamp += timestamp;
    }
    else
    {
        header->timestamp = timestamp;
        header->timestamp_delta = 0;
    }
    header->payload_len = payload_len;
    header->message_type_id = message_type_id;
    header->stream_id = ((RtmpMessageAudio == message_type_id || RtmpMessageAMF3MetaData == message_type_id || RtmpMessageAMF0MetaData == message_type_id)
                        ? 1 : stream_id);

    //uint8_t其实就是unsigned char, 直接输出(无论是ostringstream还是cout)会输出ascii码, 没有对应的ascii就会显示为空或者乱码
    LOG_TRACE << "csid = " << (uint32_t )csid << ", fmt = " << (uint32_t )fmt << ", timestamp_delta = " << header->timestamp_delta << ", timestamp = "
                << header->timestamp << ", payload_len = " << payload_len << ", message_type_id = " << (uint32_t )message_type_id
                << ", stream_id = " << stream_id;

    SsmsPacketPtr packet;
    if (packets_.find(csid) == packets_.end())
    {
        packet = std::make_shared<SsmsPacket>(payload_len);
        RtmpMessageHeaderPtr header_ext = std::make_shared<struct RtmpMessageHeader>();
        header_ext->csid = header->csid;
        header_ext->timestamp = header->timestamp;
        header_ext->payload_len = header->payload_len;
        header_ext->message_type_id = header->message_type_id;
        header_ext->stream_id = header->stream_id;
        header_ext->timestamp_delta = header->timestamp_delta;
        packet->SetExt<struct RtmpMessageHeader>(header_ext);
        packets_[csid] = packet;
    }
    else
    {
        packet = packets_[csid];
    }
    uint32_t chunk_len = (packet->to_be_received < r_chunk_size_ ? packet->to_be_received : r_chunk_size_);
    if (data->readableBytes() < offset + chunk_len)
    {
        return 1;
    }
    memcpy(packet->data + packet->payload_size_ - packet->to_be_received, p_data + offset, chunk_len);
    packet->to_be_received -= chunk_len;
    data->retrieve(offset + chunk_len);
    
    //数据接收完, 所有chunk组装完毕
    if (0 == packet->to_be_received)
    {
        packets_.erase(csid);
        return ParseAssembledMessage(packet);
    }

    return 0;
}

int SsmsRtmpMessageContext::ParseAssembledMessage(const SsmsPacketPtr &data)
{
    char *p_data = data->data;
    uint32_t offset = 0;
    int ret = 0;
    RtmpMessageHeaderPtr header = data->Ext<struct RtmpMessageHeader>();
    if (!header)
    {
        LOG_ERROR << "rtmp data error, does not have rtmp header info";
        return -1;
    }

    switch (header->message_type_id)
    {
        case RtmpMessageSetChunkSize:
            ret = ParseSetChunkSize(data);
            break;
        case RtmpMessageAcknowledgement:
            break;
        case RtmpMessageUserControl:
            ret = ParseUserControlMessage(data);
            break;
        case RtmpMessageWindowAcknowledgementSize:
            ret = ParseWindowAcknowledgementSize(data);
            break;
        case RtmpMessageAudio:
        case RtmpMessageVideo:
            ret = ProcessAudioVideo(data);
            break;
        case RtmpMessageAMF3MetaData:
        case RtmpMessageAMF0MetaData:
            ret = ParseDataMessage(data);
            break;
        case RtmpMessageAMF3Command:
            offset += 1;
        case RtmpMessageAMF0Command:
            ret = ParseCommandMessage(data, offset);
            break;
        default:
            LOG_ERROR << "unsupported message type " << (uint32_t)header->message_type_id;
            ret = -1;
            break;
    }

    return ret;
}

int SsmsRtmpMessageContext::ParseCommandMessage(const SsmsPacketPtr &data, uint32_t offset)
{
    int ret = 0;
    char *p_data = data->data;
    uint32_t len = data->payload_size_;

    //command
    if (Amf0String != *(p_data + offset++))
    {
        LOG_ERROR << "parse RTMP message erorr";
        ret = -1;
        return ret;
    }
    uint16_t str_len = ::ntohs(*(uint16_t *)(p_data + offset));
    offset += 2;
    std::string command(p_data + offset, str_len);
    offset += str_len;
    //transaction_id
    if (Amf0Number != *(p_data + offset++))
    {
        LOG_ERROR << "parse RTMP message erorr";
        ret = -1;
        return ret;
    }
    double transaction_id = 0.0;
    uint64_t temp = __builtin_bswap64(*(uint64_t *)(p_data + offset));
    memcpy(&transaction_id, &temp, sizeof(temp));
    offset += sizeof(temp);
    SsmsAmf0TypePtr amf_data;
    if (ParseAmfData(data, offset, command, amf_data) < 0)
    {
        LOG_DEBUG << "parse amf data error";
        ret = -1;
        return ret;
    }

    if ("connect" == command)
    {
        if (ConnectResponse(transaction_id, app_) < 0)
        {
            LOG_ERROR << "send rtmp connect response error";
            ret = -1;
            return ret;
        }
    }
    else if ("releaseStream" == command)
    {
        client_ = std::make_shared<SsmsPublishClient>(app_,
                                                        stream_,
                                                        live_manage_,
                                                        conn_.lock(),
                                                        std::dynamic_pointer_cast<SsmsRtmpMessageContext>(shared_from_this()),
                                                        loop_);
        ret = client_->Process(data, command, transaction_id);
    }
    else if ("FCPublish" == command)
    {
        if (!client_)
        {
            client_ = std::make_shared<SsmsPublishClient>(app_,
                                                            stream_,
                                                            live_manage_,
                                                            conn_.lock(),
                                                            std::dynamic_pointer_cast<SsmsRtmpMessageContext>(shared_from_this()),
                                                            loop_);
        }
        ret = client_->Process(data, command, transaction_id);
    }
    else if ("createStream" == command)
    {
        ret = CreateStreamResponse(transaction_id);
    }
    else if ("publish" == command)
    {
        if (!client_)
        {
            client_ = std::make_shared<SsmsPublishClient>(app_,
                                                            stream_,
                                                            live_manage_,
                                                            conn_.lock(),
                                                            std::dynamic_pointer_cast<SsmsRtmpMessageContext>(shared_from_this()),
                                                            loop_);
        }
        ret = client_->Process(data, command, transaction_id);
    }
    else if ("getStreamLength" == command)
    {
        client_ = std::make_shared<SsmsPlayClient>(app_,
                                                    stream_,
                                                    live_manage_,
                                                    conn_.lock(),
                                                    std::dynamic_pointer_cast<SsmsRtmpMessageContext>(shared_from_this()),
                                                    loop_);
        ret = client_->Process(data, command);
    }
    else if ("play" == command)
    {
        if (!client_)
        {
            client_ = std::make_shared<SsmsPlayClient>(app_,
                                                    stream_,
                                                    live_manage_,
                                                    conn_.lock(),
                                                    std::dynamic_pointer_cast<SsmsRtmpMessageContext>(shared_from_this()),
                                                    loop_);
        }
        ret = client_->Process(data, command, transaction_id);
    }
    else if ("FCUnpublish" == command)
    {
        ret = client_->Process(data, command);
    }
    else if ("FCSubscribe" == command)
    {
        //忽略
    }
    else
    {
        LOG_DEBUG << "unsupported rtmp command message: " << command;
        ret = -1;
    }

    return ret;
}

int SsmsRtmpMessageContext::ConnectResponse(double transaction_id, const std::string &app)
{
    const std::string &config_val = S_SSMSCONFIG->GetString("RTMP", "app", "");
    if (config_val.find("\"live\"") == std::string::npos)
    {
        LOG_ERROR << "app name error, " << app << "does not exist";
        return -1;
    }

    //WindowAcknowledgementSize
    SsmsPacketPtr pkt = std::make_shared<SsmsPacket>(4);
    RtmpMessageHeaderPtr header = std::make_shared<RtmpMessageHeader>();
    header->csid = 2;
    header->timestamp = 0;
    header->payload_len = 4;
    header->message_type_id = 5;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    SsmsUtils::Write4BytesBe(pkt->data, 2.5 * 1000 * 1000);
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp window acknowledgement size";

    //SetPeerBandwidth
    pkt = std::make_shared<SsmsPacket>(5);
    header = std::make_shared<RtmpMessageHeader>();
    header->csid = 2;
    header->timestamp = 0;
    header->payload_len = 5;
    header->message_type_id = 6;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    SsmsUtils::Write4BytesBe(pkt->data, 2.5 * 1000 * 1000);
    SsmsUtils::Write1Byte(pkt->data + 4, 2);
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp set peer bandwidth";

    //SetChunkSize
    pkt = std::make_shared<SsmsPacket>(4);
    header = std::make_shared<RtmpMessageHeader>();
    header->csid = 2;
    header->timestamp = 0;
    header->payload_len = 4;
    header->message_type_id = 1;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    SsmsUtils::Write4BytesBe(pkt->data, 128);
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp set chunk size";

    //connect response
    std::shared_ptr<SsmsAmf0String> command = std::make_shared<SsmsAmf0String>();
    command->SetValue("_result");
    std::shared_ptr<SsmsAmf0Number> trans_id = std::make_shared<SsmsAmf0Number>();
    trans_id->SetValue(transaction_id);
    SsmsAmf0TypePtr object1 = std::make_shared<SsmsAmf0Object>();
    //fmsVer
    SsmsAmf0TypePtr propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("FMS/3,5,3,888");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("fmsVer", propertie);
    //capabilities
    propertie = std::make_shared<SsmsAmf0Number>();
    std::dynamic_pointer_cast<SsmsAmf0Number>(propertie)->SetValue(127);
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("capabilities", propertie);
    //mode
    propertie = std::make_shared<SsmsAmf0Number>();
    std::dynamic_pointer_cast<SsmsAmf0Number>(propertie)->SetValue(1);
    std::dynamic_pointer_cast<SsmsAmf0Object>(object1)->SetValue("mode", propertie);

    SsmsAmf0TypePtr object2 = std::make_shared<SsmsAmf0Object>();
    //level
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("status");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object2)->SetValue("level", propertie);
    //code
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("NetConnection.Connect.Success");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object2)->SetValue("code", propertie);
    //description
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("Connection succeeded");
    std::dynamic_pointer_cast<SsmsAmf0Object>(object2)->SetValue("description", propertie);
    //objectEncoding
    propertie = std::make_shared<SsmsAmf0Number>();
    std::dynamic_pointer_cast<SsmsAmf0Number>(propertie)->SetValue(0);
    std::dynamic_pointer_cast<SsmsAmf0Object>(object2)->SetValue("objectEncoding", propertie);
    
    SsmsAmf0TypePtr ecma_array = std::make_shared<SsmsAmf0EcmaArray>();
    //version
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("3,5,3,888");
    std::dynamic_pointer_cast<SsmsAmf0EcmaArray>(ecma_array)->SetValue("version", propertie);
    //server
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("ssms(simple streaming media server)");
    std::dynamic_pointer_cast<SsmsAmf0EcmaArray>(ecma_array)->SetValue("server", propertie);
    //ssms_version
    propertie = std::make_shared<SsmsAmf0String>();
    std::dynamic_pointer_cast<SsmsAmf0String>(propertie)->SetValue("0.2.3");
    std::dynamic_pointer_cast<SsmsAmf0EcmaArray>(ecma_array)->SetValue("ssms_version", propertie);

    std::string status("_result");
    uint32_t pkt_len = command->EncodeSize() + trans_id->EncodeSize() + object1->EncodeSize() + object2->EncodeSize() + ecma_array->EncodeSize();
    pkt = std::make_shared<SsmsPacket>(pkt_len);
    header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = pkt_len;
    header->message_type_id = 20;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    uint16_t offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += trans_id->Encode(pkt->data + offset);
    offset += object1->Encode(pkt->data + offset);
    offset += object2->Encode(pkt->data + offset);
    offset += ecma_array->Encode(pkt->data + offset);
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp connect response";

    return 0;
}

bool SsmsRtmpMessageContext::BuildChunk(const SsmsPacketPtr &pkt, bool fmt0)
{
    RtmpMessageHeaderPtr header = pkt->Ext<RtmpMessageHeader>();
    if (!header)
    {
        LOG_ERROR << "rtmp header is nullptr";
        return false;
    }

    //判断packet所需的头部总大小是否超过sending_的剩余空间大小, 超过则下次再发
    int chunk_nums = pkt->payload_size_ / s_chunk_size_ + ((pkt->payload_size_ % s_chunk_size_) == 0 ? 0 : 1);
    int rtmp_header_total_size = 18;
    if (chunk_nums > 1)
    {
        rtmp_header_total_size += (chunk_nums - 1) * 8;
    }
    //basic header最大3 bytes, extended timaestamp 4 bytes
    if (sending_curr_ + rtmp_header_total_size >= sizeof(sending_))
    {
        LOG_DEBUG << "rtmp header data is large than buffer";
        return false;
    }

    bool has_csid = true;
    RtmpMessageHeaderPtr prev = prev_send_headers_[header->csid];
    if (!prev)
    {
        has_csid = false;
        prev_send_headers_[header->csid] = std::make_shared<struct RtmpMessageHeader>();
        prev = prev_send_headers_[header->csid];
    }
    header->timestamp_delta = header->timestamp - prev->timestamp;

    uint8_t fmt = 0;
    uint8_t msg_header_size = 0;
    uint32_t prev_timestamp_delta = prev_timestamp_deltas_[header->csid];
    if (fmt0 || !has_csid || prev->timestamp > header->timestamp)
    {
        fmt = RTMPFmt0;
        msg_header_size = 11;
    }
    else if (header->timestamp_delta != prev_timestamp_delta && header->payload_len != prev->payload_len && header->message_type_id != prev->message_type_id)
    {
        fmt = RTMPFmt1;
        msg_header_size = 7;
    }
    else if (header->timestamp_delta != prev_timestamp_delta)
    {
        fmt = RTMPFmt2;
        msg_header_size = 3;
    }
    else
    {
        fmt = RTMPFmt3;
    }

    if (fmt != RTMPFmt0)
    {
        prev_timestamp_deltas_[header->csid] = prev_timestamp_delta;
    }

    //第一个chunk的头部
    uint32_t offset_header_start = sending_curr_;
    char *p = sending_;
    if (header->csid <= 63)
    {
        *(p + sending_curr_++) = (fmt << 6 | header->csid);
    }
    else if (header->csid <= 319)
    {
        *(p + sending_curr_++) = (fmt << 6 | 0x00);
        *(p + sending_curr_++) = header->csid - 64;
    }
    else if (header->csid <= 65599)
    {
        *(p + sending_curr_++) = (fmt << 6 | 0x01);
        *(uint16_t *)(p + sending_curr_) = (uint16_t)(header->csid - 64);
        sending_curr_ += 2;
    }
    else
    {
        LOG_ERROR << "csid error, greater than 65599";
        return false;
    }
    if (RTMPFmt0 == fmt)
    {
        uint32_t timestamp;
        if (header->timestamp >= 0xFFFFFF)
        {
            timestamp = 0xFFFFFF;
        }
        else
        {
            timestamp = header->timestamp;
        }
        SsmsUtils::Write3BytesBe(p + sending_curr_, timestamp);
        sending_curr_ += 3;
        SsmsUtils::Write3BytesBe(p + sending_curr_, header->payload_len);
        sending_curr_ += 3;
        SsmsUtils::Write1Byte(p + sending_curr_, header->message_type_id);
        sending_curr_ += 1;
        SsmsUtils::Write4BytesLe(p + sending_curr_, header->stream_id);
        sending_curr_ += 4;
        if (header->timestamp >= 0xFFFFFF)
        {
            SsmsUtils::Write4BytesBe(p + sending_curr_, header->timestamp);
            sending_curr_ += 4;
        }
    }
    else if (RTMPFmt1 == fmt)
    {
        uint32_t timestamp;
        if (header->timestamp_delta >= 0xFFFFFF)
        {
            timestamp = 0xFFFFFF;
        }
        else
        {
            timestamp = header->timestamp_delta;
        }
        SsmsUtils::Write3BytesBe(p + sending_curr_, timestamp);
        sending_curr_ += 3;
        SsmsUtils::Write3BytesBe(p + sending_curr_, header->payload_len);
        sending_curr_ += 3;
        SsmsUtils::Write1Byte(p + sending_curr_, header->message_type_id);
        sending_curr_ += 1;
        if (header->timestamp_delta >= 0xFFFFFF)
        {
            SsmsUtils::Write4BytesBe(p + sending_curr_, header->timestamp_delta);
            sending_curr_ += 4;
        }
    }
    else if (RTMPFmt2 == fmt)
    {
        uint32_t timestamp;
        if (header->timestamp_delta >= 0xFFFFFF)
        {
            timestamp = 0xFFFFFF;
        }
        else
        {
            timestamp = header->timestamp_delta;
        }
        SsmsUtils::Write3BytesBe(p + sending_curr_, timestamp);
        sending_curr_ += 3;
        if (header->timestamp_delta >= 0xFFFFFF)
        {
            SsmsUtils::Write4BytesBe(p + sending_curr_, header->timestamp_delta);
            sending_curr_ += 4;
        }
    }
    else
    {
        if (prev_timestamp_delta >= 0xFFFFFF)
        {
            SsmsUtils::Write4BytesBe(p + sending_curr_, prev_timestamp_delta);
            sending_curr_ += 4;
        }
    }
    BufferNodePtr node = std::make_shared<struct iovec>();
    node->iov_base = p + offset_header_start;
    node->iov_len = sending_curr_ - offset_header_start;
    sending_nodes_.emplace_back(std::move(node));
    prev->message_type_id = header->message_type_id;
    prev->payload_len = header->payload_len;
    prev->stream_id = header->stream_id;
    prev->timestamp = header->timestamp;
    prev->timestamp_delta = header->timestamp_delta;
    offset_header_start = sending_curr_;

    uint32_t offset_payload_end = 0;
    fmt = RTMPFmt3;
    while (offset_payload_end < pkt->payload_size_)
    {

        uint32_t min_len = (pkt->payload_size_ - offset_payload_end < s_chunk_size_ ? pkt->payload_size_ - offset_payload_end : s_chunk_size_);
        node = std::make_shared<struct iovec>();
        node->iov_base = pkt->data + offset_payload_end;
        node->iov_len = min_len;
        sending_nodes_.emplace_back(std::move(node));
        offset_payload_end += min_len;
        if (offset_payload_end == pkt->payload_size_)
        {
            break;
        }

        //basic header最大3 bytes, fmt3的message header 1 byte, extended timaestamp 4 bytes
        if (sending_curr_ + 8 >= sizeof(sending_))
        {
            LOG_ERROR << "rtmp header data is large than buffer";
            return false;
        }

        //后续chunk使用fmt3
        //basic header
        if (header->csid <= 63)
        {
            *(p + sending_curr_++) = (fmt << 6 | header->csid);
        }
        else if (header->csid <= 319)
        {
            *(p + sending_curr_++) = (fmt << 6 | 0x00);
            *(p + sending_curr_++) = header->csid - 64;
        }
        else
        {
            *(p + sending_curr_++) = (fmt << 6 | 0x01);
            *(uint16_t *)(p + sending_curr_) = (uint16_t)(header->csid - 64);
            sending_curr_ += 2;
        }

        //message header
        if (header->timestamp_delta >= 0xFFFFFF)
        {
            SsmsUtils::Write4BytesBe(p + sending_curr_, header->timestamp_delta);
            sending_curr_ += 4;
        }
        node = std::make_shared<struct iovec>();
        node->iov_base = p + offset_header_start;
        node->iov_len = sending_curr_ - offset_header_start;
        sending_nodes_.emplace_back(std::move(node));
        offset_header_start = sending_curr_;
    }

    sending_pkts_.emplace_back(std::move(pkt));
    return true;
}

void SsmsRtmpMessageContext::SendNodes()
{
    conn_.lock()->SendNodes(sending_nodes_);
}

int SsmsRtmpMessageContext::ParseSetChunkSize(const SsmsPacketPtr &data)
{
    r_chunk_size_ = ::ntohl(*(uint32_t *)data->data);
    LOG_DEBUG << "client send chunk size " << r_chunk_size_;
    return 0;
}

int SsmsRtmpMessageContext::ParseUserControlMessage(const SsmsPacketPtr &data)
{
    char *p = data->data;
    uint32_t offset = 0;
    uint32_t remaining_length = data->payload_size_;
    if (remaining_length < 6)
    {
        LOG_DEBUG << "user control message format error";
        return -1;
    }
    int ret = 0;
    uint16_t event_type = ::ntohs(*(uint16_t *)(p + offset));
    switch (event_type)
    {
        case UserControlMsgSetBufferLength:
            break;
        default:
            LOG_DEBUG << "unsupported user control message type " << event_type;
            ret = -1;
            break;
    }

    return ret;
}

int SsmsRtmpMessageContext::ParseWindowAcknowledgementSize(const SsmsPacketPtr &data)
{
    client_window_ack_size_ = ::ntohl(*(uint32_t *)data->data);
    LOG_DEBUG << "client window acknowledgement size " << client_window_ack_size_;
    return 0;
}

int SsmsRtmpMessageContext::ProcessAudioVideo(const SsmsPacketPtr &data)
{
    return client_->Process(data, "");
}

int SsmsRtmpMessageContext::CreateStreamResponse(double trans_id)
{
    std::shared_ptr<SsmsAmf0String> command = std::make_shared<SsmsAmf0String>();
    command->SetValue("_result");
    std::shared_ptr<SsmsAmf0Number> transaction_id = std::make_shared<SsmsAmf0Number>();
    transaction_id->SetValue(trans_id);
    std::shared_ptr<SsmsAmf0NULL> null = std::make_shared<SsmsAmf0NULL>();
    std::shared_ptr<SsmsAmf0Number> stream_id = std::make_shared<SsmsAmf0Number>();
    stream_id->SetValue(1.0);
    int payload_size = command->EncodeSize() + transaction_id->EncodeSize() + null->EncodeSize() + stream_id->EncodeSize();
    SsmsPacketPtr pkt = std::make_shared<SsmsPacket>(payload_size);
    int offset = 0;
    offset += command->Encode(pkt->data + offset);
    offset += transaction_id->Encode(pkt->data + offset);
    offset += null->Encode(pkt->data + offset);
    offset += stream_id->Encode(pkt->data + offset);
    RtmpMessageHeaderPtr header = std::make_shared<RtmpMessageHeader>();
    header->csid = 3;
    header->timestamp = 0;
    header->payload_len = payload_size;
    header->message_type_id = 20;
    header->stream_id = 0;
    pkt->SetExt<RtmpMessageHeader>(std::move(header));
    PostMessage(std::move(pkt), true);
    LOG_DEBUG << "send rtmp createStream response";
    return 0;
}

int SsmsRtmpMessageContext::ParseDataMessage(const SsmsPacketPtr &data)
{
    return client_->Process(data, "");
}