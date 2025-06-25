#pragma once

#include <memory>
#include <stdint.h>

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

        class SsmsStream;
        using SsmsStreamPtr = std::shared_ptr<SsmsStream>;

        class SsmsCodecHeader;
        using SsmsCodecHeaderPtr = std::shared_ptr<SsmsCodecHeader>;

        class SsmsGopManagment;
        using SsmsGopManagmentPtr = std::shared_ptr<SsmsGopManagment>;

        class SsmsTimestampCorrector;
        using SsmsTimestampCorrectorPtr = std::shared_ptr<SsmsTimestampCorrector>;

        //音频最大间隔, 单位毫秒, 超过此值需要纠正时间戳
        const int AUDIO_MAX_DELTA = 67;
        //视频最大间隔, 单位毫秒, 超过此值需要纠正时间戳
        const int VIDEO_MAX_DELTA = 87;
        //音频默认间隔
        const int AUDIO_DEFAULT_DELTA = 23;
        //视频默认间隔
        const int VIDEO_DEFAULT_DELTA = 40;

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

        struct GopItemInfo
        {
            GopItemInfo(uint64_t index, uint32_t timestamp)
            : key_frame_index(index)
            , key_frame_timestamp(timestamp)
            {

            }

            uint64_t key_frame_index;
            uint32_t key_frame_timestamp;
        };
    }
}