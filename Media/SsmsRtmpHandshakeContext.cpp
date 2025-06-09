#include <random>
#include <arpa/inet.h>
#include "SsmsRtmpHandshakeContext.h"
#include "Base/SsmsLogStream.h"
#include "SsmsRtmpMessageContext.h"

using namespace ssms::media;

SsmsRtmpHandshakeContext::SsmsRtmpHandshakeContext(const TcpConnectionPtr &conn)
: conn_(conn)
{

}

SsmsRtmpHandshakeContext::~SsmsRtmpHandshakeContext()
{

}

int SsmsRtmpHandshakeContext::Parse(const SsmsBufferPtr &data)
{
    switch (state_)
    {
        case RtmpHandshakeWaitC0:
            return OnC0(data);
        case RtmpHandshakeWaitC1:
            return OnC1(data);
        case RtmpHandshakeWaitC2:
            return OnC2(data);
        default:
            break;
    }

    LOG_DEBUG << "RTMP context state error";
    return -1;
}

int SsmsRtmpHandshakeContext::OnC0(const SsmsBufferPtr &data)
{
    if (data->readableBytes() < 1)
    {
        return 1;
    }

    if (data->readInt8() != 0x03)
    {
        LOG_DEBUG << "RTMP protocol version error, must be 3";
        return -1;
    }
    
    if (data->readableBytes() >= 1536)
    {
        return OnC1(data);
    }
    
    SendS0S1S2(false, 0);
    state_ = RtmpHandshakeWaitC1;

    return 1;
}

int SsmsRtmpHandshakeContext::OnC1(const SsmsBufferPtr &data)
{
    uint32_t timestamp =  data->readInt32();
    data->retrieve(1532);
    SendS0S1S2(true, timestamp);
    state_ = RtmpHandshakeWaitC2;
    return 0;
}

int SsmsRtmpHandshakeContext::OnC2(const SsmsBufferPtr &data)
{
    if (data->readableBytes() < 1536)
    {
        return 1;
    }

    LOG_DEBUG << "handshake complete";
    std::dynamic_pointer_cast<SsmsRtmpMessageContext>(conn_.lock()->context_)->state_ = RtmpMessageControl;
    data->retrieve(1536);

    return 0;
}

void SsmsRtmpHandshakeContext::SendS0S1S2(bool has_c1, uint32_t timestamp)
{
    int offset = 0;
    s0s1s2_[offset++] = 0x03;
    *(uint32_t *)(s0s1s2_ + offset) = ::htonl((int)(time(nullptr) >> 32));
    offset += 4;
    *(uint32_t *)(s0s1s2_ + offset) = 0;
    offset += 4;
    for (int i = 0; i < 1528; i++)
    {
        s0s1s2_[offset++] = GenRandom();
    }
    if (has_c1)
    {
        *(uint32_t *)(s0s1s2_ + offset) = ::htonl((int)(time(nullptr) >> 32));
        offset += 4;
        *(uint32_t *)(s0s1s2_ + offset) = timestamp;
        offset += 4;
        for (int i = 0; i < 1528; i++)
        {
            s0s1s2_[offset++] = GenRandom();
        }
    }
    conn_.lock()->SendPktInLoop(s0s1s2_, offset);
}

uint8_t SsmsRtmpHandshakeContext::GenRandom()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<> rand(0, 255);
    return rand(mt);
}
