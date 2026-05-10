#pragma once

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
}
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsAACDecoder
        {
        public:
            SsmsAACDecoder()
            {
                errro_buffer_[0] = '\0';
            };
            ~SsmsAACDecoder();

            bool Init(int sample_rate, int frame_size, int channels);
            bool Decode(uint8_t *data, int len, SsmsUdpPktPtr &out_data);
        private:
            void Free();    

            AVCodecContext *dec_ctx_{nullptr};
            AVPacket *a_pkt_{nullptr};
            AVFrame *a_decoded_frame_{nullptr};
            char errro_buffer_[AV_ERROR_MAX_STRING_SIZE];
        };
    }
}