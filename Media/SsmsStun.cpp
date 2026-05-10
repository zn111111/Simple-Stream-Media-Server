#include <arpa/inet.h>
#include <cstring>
#include <openssl/hmac.h>
#include "SsmsStun.h"
#include "Base/SsmsLogStream.h"
#include "Base/SsmsUtils.h"
#include "Network/SsmsUdpSocket.h"
#include "Network/SsmsUdpPkt.h"
#include "Network/SsmsNetAddress.h"
#include "SsmsWebrtcServer.h"
#include "SsmsWebrtcPlayClient.h"

using namespace ssms::media;
using namespace ssms::base;
using namespace ssms::nw;

static uint32_t MAGIC_COOKIE = 0x2112A442;

enum StunMsgtype
{
    BindRequest = 0x0001,
    BindResponse = 0x0101
};

enum StunAttributeType
{
    UserName = 0x0006,
    MessageIntegrity = 0x0008,
    XorMappedAddress = 0x0020,
    Fingerprint = 0x8028
};

int SsmsStun::OnMessage(const SsmsWebrtcServerPtr &rtc_server, const SsmsUdpSocketPtr &udp_socket, const SsmsNetAddressPtr &client_addr, const SsmsUdpPktPtr &in_pkt)
{
    int ret = 0;
    std::string password;
    if ((ret = Decode(rtc_server, client_addr, in_pkt->data_, in_pkt->len_, password)) < 0)
    {
        LOG_ERROR << "stun packet decode failed";
        return -1;
    }
    else if (1 == ret)
    {
        SsmsUdpPktPtr out_pkt;
        if (Encode(password, client_addr, out_pkt) < 0)
        {
            LOG_ERROR << "stun packet encode failed";
            return -1;
        }
        udp_socket->SendPacket(out_pkt);
    }

    return 0;
}

int SsmsStun::Decode(SsmsWebrtcServerPtr rtc_server, const SsmsNetAddressPtr &client_addr, const char *data, int len, std::string &password)
{
    int ret = 0;
    if (!data || len < 20)
    {
        LOG_ERROR << "input data error";
        ret = -1;
        return ret;
    }

    int offset = 0;
    const char *p = data;
    int msg_type = ::ntohs(*(uint16_t *)data);
    if (msg_type != BindRequest)
    {
        LOG_ERROR << "unsupported stun message type " << msg_type;
        ret = -1;
        return ret;
    }
    offset += 2;
    int msg_len = ::ntohs(*(uint16_t *)(data + offset));
    if (msg_len  + 20 != len)
    {
        LOG_ERROR << "stun message format error, message length " << msg_len;
        ret = -1;
        return ret;
    }
    offset += 2;
    if (::ntohl(*(uint32_t *)(data + offset)) != MAGIC_COOKIE)
    {
        LOG_ERROR << "stun message format error, magic cookie is not equal to " << MAGIC_COOKIE;
        ret = -1;
        return ret;
    }
    offset += 4;
    transaction_id_.assign(data + offset, 12);
    offset += 12;

    while (offset < len)
    {
        if (offset + 2 > len)
        {
            LOG_ERROR << "stun message format error, length is not enough";
            ret = -1;
            return ret;
        }
        uint16_t attr_type = ::ntohs(*(uint16_t *)(data + offset));
        offset += 2;
        if (offset + 2 > len)
        {
            LOG_ERROR << "stun message format error, length is not enough";
            ret = -1;
            return ret;
        }
        uint16_t attr_len = ::ntohs(*(uint16_t *)(data + offset));
        offset += 2;
        if (offset + attr_len > len)
        {
            LOG_ERROR << "stun message format error, length is not enough";
            ret = -1;
            return ret;
        }
        switch (attr_type)
        {
            case UserName:
            {
                std::string username(data + offset, attr_len);
                std::vector<std::string> v = SsmsUtils::Split(username, ":");
                if (v.size() != 2)
                {
                    LOG_ERROR << "stun message ufrag format error";
                    ret = -1;
                    return ret;
                }
                local_ufrag_ = v[0];
                remote_ufrag_ = v[1];
            }
                break;
            case MessageIntegrity:
            {
                SsmsWebrtcPlayClientPtr rtc_client = rtc_server->GetClient(local_ufrag_);
                if (!rtc_client)
                {
                    //说明已经处理过了stun bind reques
                    return 0;
                }
                password = rtc_client->GetLocalPassword();
                rtc_server->DeleteClient(local_ufrag_);
                rtc_server->AddClient(client_addr->GetFourTuple(), rtc_client);
                rtc_client->SetClientAddr(client_addr);

                uint16_t ori_msg_len = ntohs(*(uint16_t *)(data + 2));
                SsmsUtils::Write2BytesBe(const_cast<char *>(data + 2), offset);
                char mi[21] = {'\0'};
                CalHmac(password, data, offset - 4, mi);
                SsmsUtils::Write2BytesBe(const_cast<char *>(data + 2), ori_msg_len);
                if (memcmp(data + offset, mi, 20))
                {
                    LOG_ERROR << "stun packet might have been modified";
                    return -1;
                }
            }
                break;
            default:
                break;
        }
        offset += attr_len;
        //每个属性都必须是4的整数倍, 不够的在value后补0
        offset += (4 - attr_len % 4) % 4;
    }
    LOG_DEBUG << "decode stun bind request, message length: " << msg_len << ", transaction_id: " << transaction_id_
                << ", local_ufrag: " << local_ufrag_ << ", remote_ufrag: " << remote_ufrag_;

    return ret;
}

int SsmsStun::Encode(const std::string &password, const SsmsNetAddressPtr &client_addr, SsmsUdpPktPtr &pkt)
{
    //响应头
    uint32_t offset = 0;
    pkt = std::make_shared<SsmsUdpPkt>(512);
    SsmsUtils::Write2BytesBe(pkt->data_, BindResponse);
    offset += 2;
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, 0);
    offset += 2;
    SsmsUtils::Write4BytesBe(pkt->data_ + offset, MAGIC_COOKIE);
    offset += 4;
    SsmsUtils::WriteNBytes(pkt->data_ + offset, transaction_id_.data(), transaction_id_.size());
    offset += transaction_id_.size();

    //属性: 用户名
    std::string username = local_ufrag_ + ":" + remote_ufrag_;
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, UserName);
    offset += 2;
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, username.size());
    offset += 2;
    SsmsUtils::WriteNBytes(pkt->data_ + offset, username.data(), username.size());
    offset += username.size();
    uint8_t padding_nums = (4 - username.size() % 4) % 4;
    if (padding_nums)
    {
        memset(pkt->data_ + offset, 0, padding_nums);
        offset += padding_nums;
    }

    //属性: 异或后的地址
    uint32_t client_ip = 0;
    if (client_addr->Ipv4())
    {
        if (::inet_pton(AF_INET, client_addr->GetStringIp().c_str(), &client_ip) <= 0)
        {
            LOG_ERROR << "inet_pton convert str ip to interger error";
            return -1;
        }
        client_ip = ::ntohl(client_ip);
    }
    else
    {
        LOG_ERROR << "input struct sockaddr_in6 data error, only ipv4 is supported";
        return -1;
    }
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, XorMappedAddress);
    offset += 2;
    char *p_addr_len = pkt->data_ + offset;
    uint32_t addr_len = 0;
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, 0);
    offset += 2;
    SsmsUtils::Write1Byte(pkt->data_ + offset, 0);
    offset += 1;
    addr_len += 1;
    SsmsUtils::Write1Byte(pkt->data_ + offset, 0x01);
    offset += 1;
    addr_len += 1;
    uint16_t xor_port = client_addr->GetPort() ^ (MAGIC_COOKIE >> 16);
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, xor_port);
    offset += 2;
    addr_len += 2;
    uint32_t xor_addr = client_ip ^ MAGIC_COOKIE;
    SsmsUtils::Write4BytesBe(pkt->data_ + offset, xor_addr);
    offset += 4;
    addr_len += 4;
    padding_nums = (4 - offset % 4) % 4;
    if (padding_nums)
    {
        memset(pkt->data_ + offset, 0, padding_nums);
        offset += padding_nums;
    }
    SsmsUtils::Write2BytesBe(p_addr_len, addr_len);

    //属性: 消息完整性
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, MessageIntegrity);
    offset += 2;
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, 20);
    offset += 2;
    SsmsUtils::Write2BytesBe(pkt->data_ + 2, offset);
    CalHmac(password, pkt->data_, offset - 4, pkt->data_ + offset);
    offset += 20;

    //属性: Fingerprint
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, Fingerprint);
    offset += 2;
    SsmsUtils::Write2BytesBe(pkt->data_ + offset, 4);
    offset += 2;
    SsmsUtils::Write2BytesBe(pkt->data_ + 2, offset - 16);
    uint32_t crc32 = SsmsUtils::Crc32Ieee(pkt->data_, offset - 4) ^ 0x5354554E;
    SsmsUtils::Write4BytesBe(pkt->data_ + offset, crc32);
    offset += 4;

    pkt->len_ = offset;

    return 0;
}

std::string SsmsStun::GetLocalUfrag() const
{
    return local_ufrag_;
}

std::string SsmsStun::GetRemoteUfrag() const
{
    return remote_ufrag_;
}

void SsmsStun::CalHmac(const std::string &password, const char *src_data, int src_len, char *dst_data)
{
    unsigned digest_len;
#if OPENSSL_VERSION_NUMBER > 0X10100000L
    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, password.c_str(), password.size(), EVP_sha1(), nullptr);
    HMAC_Update(ctx, (const unsigned char *)src_data, src_len);
    HMAC_Final(ctx, (unsigned char *)dst_data, &digest_len);
    HMAC_CTX_free(ctx);
#else
    HMAC_CTX ctx;
    HMAC_Init_init(&ctx);
    HMAC_Init_ex(ctx, password.c_str(), password.size(), EVP_sha1(), nullptr);
    HMAC_Update(ctx, (const unsigned char *)src_data, src_len);
    HMAC_Final(ctx, (unsigned char *)dst_data, &digest_len);
    HMAC_CTX_cleanup(&ctx);
#endif
}