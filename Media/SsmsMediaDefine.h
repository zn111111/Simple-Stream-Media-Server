#pragma once

#include <memory>

namespace ssms
{
    namespace media
    {
        class SsmsContext;
        using SsmsContextPtr = std::shared_ptr<SsmsContext>;

        class SsmsRtmpMessageContext;
        using SsmsRtmpMessageContextPtr = std::shared_ptr<SsmsRtmpMessageContext>;

        struct SsmsPacket;
        using SsmsPacketPtr = std::shared_ptr<struct SsmsPacket>;

        class SsmsRtmpHandshakeContext;
        using SsmsRtmpHandshakeContextPtr = std::shared_ptr<SsmsRtmpHandshakeContext>;

        struct RtmpMessageHeader;
        using RtmpMessageHeaderPtr = std::shared_ptr<struct RtmpMessageHeader>;

        class SsmsAmf0Type;
        using SsmsAmf0TypePtr = std::shared_ptr<SsmsAmf0Type>;

        class SsmsClient;
        using SsmsClientPtr = std::shared_ptr<SsmsClient>;

        class SsmsPublishClient;
        using SsmsPublishClientPtr = std::shared_ptr<SsmsPublishClient>;

        class SsmsPlayClient;
        using SsmsPlayClientPtr = std::shared_ptr<SsmsPlayClient>;

        enum RtmpHandshakeState
        {
            RtmpHandshakeWaitC0,                    //等待C0
            RtmpHandshakeWaitC1,                    //等待C1
            RtmpHandshakeWaitC2,                    //等待C2
            RtmpHandshakeComplete                   //握手完成
        };

        enum RtmpMessageState
        {
            RtmpMessageHandshake,
            RtmpMessageControl
        };

        enum RtmpMessageType
        {
            RtmpMessageSetChunkSize = 1,
            RtmpMessageAcknowledgement = 3,
            RtmpMessageUserControl = 4,
            RtmpMessageWindowAcknowledgementSize = 5,
            RtmpMessageAudio = 8,
            RtmpMessageVideo = 9,
            RtmpMessageAMF3MetaData = 15,
            RtmpMessageAMF0MetaData = 18,
            RtmpMessageAMF3Command = 17,
            RtmpMessageAMF0Command = 20
        };

        enum Amf0Type
        {
            Amf0Number = 0,
            Amf0Bool = 1,
            Amf0String = 2,
            Amf0Object = 3,
            Amf0Null = 5,
            Amf0Undefined = 6,
            Amf0EcmaArray = 8
        };

        enum RTMPFmt
        {
            RTMPFmt0,
            RTMPFmt1,
            RTMPFmt2,
            RTMPFmt3
        };

        enum FlvAudioFormat
        {
            FlvAudioFormatAAC = 10
        };

        enum FlvVideoCodec
        {
            FlvVideoCodecAVC = 7
        };

        enum POrCType
        {
            POrCTypeProducer,
            POrCTypeConsumer,
            POrCTypeDefault
        };

        enum UserControlMsgType
        {
            UserControlMsgSetBufferLength = 3
        };

        struct RtmpMessageHeader
        {
            uint32_t csid{0};
            uint32_t timestamp{0};
            uint32_t payload_len{0};
            uint8_t message_type_id{0};
            uint32_t stream_id{0};
            uint32_t timestamp_delta{0};
        };
    }
}