#include "SsmsAACDecoder.h"
#include "Base/SsmsLogStream.h"

static FILE *fp = fopen("./audio.pcm", "wb");

SsmsAACDecoder::~SsmsAACDecoder()
{
    Free();
}

bool SsmsAACDecoder::Init(int sample_rate, int frame_size, int channels)
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec)
    {
        LOG_ERROR << "avcodec_find_decoder failed";
        return false;
    }
    dec_ctx_ = avcodec_alloc_context3(codec);
    if (!dec_ctx_)
    {
        LOG_ERROR << "avcodec_alloc_context3 failed";
        return false;
    }
    dec_ctx_->sample_rate = 44100;
    dec_ctx_->channels = 2;
    av_channel_layout_default(&dec_ctx_->ch_layout, 2);
    dec_ctx_->sample_fmt = AV_SAMPLE_FMT_S16;
    if (avcodec_open2(dec_ctx_, codec, nullptr) < 0)
    {
        LOG_ERROR << "avcodec_open2 failed";
        return false;
    }
    a_pkt_ = av_packet_alloc();
    if (!a_pkt_)
    {
        LOG_ERROR << "av_packet_alloc failed";
        return false;
    }
    a_decoded_frame_ = av_frame_alloc();
    if (!a_decoded_frame_)
    {
        LOG_ERROR << "av_frame_alloc failed";
        return false;
    }
    // a_decoded_frame_->sample_rate = sample_rate;
    // a_frame_->nb_samples = frame_size;
    // a_frame_->channels = channels;
    // av_channel_layout_copy(&a_frame_->ch_layout, ch_layout);
    // int ret = av_frame_get_buffer(a_frame_, 0);
    // if (ret < 0)
    // {
    //     LOG_ERROR << "av_frame_get_buffer failed, " << av_make_error_string(errro_buffer_, AV_ERROR_MAX_STRING_SIZE, ret);
    //     return false;
    // }

    return true;
}

bool SsmsAACDecoder::Decode(uint8_t *data, int len, SsmsUdpPktPtr &out_data)
{
    a_pkt_->data = data;
    a_pkt_->size = len;
    int ret = avcodec_send_packet(dec_ctx_, a_pkt_);
    if (0 != ret)
    {
        LOG_ERROR << "avcodec_send_packet failed, " << av_make_error_string(errro_buffer_, AV_ERROR_MAX_STRING_SIZE, ret);
        return false;
    }

    while (true)
    {
        ret = avcodec_receive_frame(dec_ctx_, a_decoded_frame_);
        if (ret < 0)
        {
            if (AVERROR_EOF == ret || AVERROR(EAGAIN) == ret)
            {
                break;
            }

            LOG_ERROR << "avcodec_receive_frame failed, " << av_make_error_string(errro_buffer_, AV_ERROR_MAX_STRING_SIZE, ret);
            return false;
        }

        /*测试*/
        if (av_sample_fmt_is_planar(dec_ctx_->sample_fmt))
        {
            for (int i = 0; i < 2; i++)
            {
                int buffer_size = av_samples_get_buffer_size(nullptr, 2, a_decoded_frame_->nb_samples, dec_ctx_->sample_fmt, 1);
                fwrite(a_decoded_frame_->data[i], 1, buffer_size, fp);
            }
        }
        else
        {
            LOG_ERROR << "is not plannar";
            return false;
        }
    }

    return true;
}

void SsmsAACDecoder::Free()
{
    avcodec_free_context(&dec_ctx_);
    av_packet_free(&a_pkt_);
    av_frame_free(&a_decoded_frame_);
}