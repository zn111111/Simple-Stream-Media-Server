#pragma once

#include <stdint.h>
#include "SsmsRtmpMessageContext.h"
#include "SsmsMediaDefine.h"

namespace ssms
{
    namespace media
    {
        struct SsmsPacket
        {
        public:
            SsmsPacket(uint32_t size)
            : data(new char[size])
            , payload_size_(size)
            , to_be_received(size)
            {

            }
            ~SsmsPacket()
            {
                if (data)
                {
                    delete[] data;
                    data = nullptr;
                }
            }

            template <typename T>
            void SetExt(std::shared_ptr<void> ext)
            {
                ext_ = ext;
            }

            template <typename T>
            std::shared_ptr<T> Ext()
            {
                return std::static_pointer_cast<T>(ext_);
            }

            bool IsAudio()
            {
                return (payload_size_ > 2 && FlvAudioFormatAAC == ((*data >> 4) & 0x0F));
            }

            bool IsVideo()
            {
                return (payload_size_ > 2 && FlvVideoCodecAVC == (*data & 0x0F));
            }

            bool IsMeta()
            {
                RtmpMessageHeaderPtr header = Ext<RtmpMessageHeader>();
                return (header && (RtmpMessageAMF3MetaData == header->message_type_id || RtmpMessageAMF0MetaData == header->message_type_id));
            }

            bool IsAudioSequenceHeader()
            {
                return (payload_size_ > 2 && FlvAudioFormatAAC == ((*data >> 4) & 0x0F) && 0 == *(data + 1));
            }

            bool IsVideoSequenceHeader()
            {
                return (payload_size_ > 2 && FlvVideoCodecAVC == (*data & 0x0F) && 0 == *(data + 1));
            }

            bool IsKeyFrame()
            {
                return IsVideo() && (((*data >> 4) & 0x0F) == 1);
            }

            uint32_t Timestamp()
            {
                RtmpMessageHeaderPtr header = Ext<RtmpMessageHeader>();
                return header->timestamp;
            }

            void SetTimestamp(uint32_t timestamp)
            {
                RtmpMessageHeaderPtr header = Ext<RtmpMessageHeader>();
                header->timestamp = timestamp;
            }

            char *data{nullptr};
            uint32_t payload_size_{0};
            //待接收的数据, 该值为0时数据才算完整
            uint32_t to_be_received{0};
            //额外数据, 例如rtmp的头部
            //因为这是一个通用结构体, 像http协议就没有rtmp这样的头部, 因此使用void指针
            std::shared_ptr<void> ext_;
            //数据包索引
            uint32_t index_{0};
        };
    }
}