#include "SsmsDtls.h"
#include "Base/SsmsLogStream.h"
#include "Network/SsmsUdpPkt.h"
#include "Network/SsmsUdpSocket.h"

using namespace ssms::media;

SsmsDtls::SsmsDtls()
{

}

SsmsDtls::~SsmsDtls()
{
    if (ssl_context_)
    {
        SSL_CTX_free(ssl_context_);
        ssl_context_ = nullptr;
    }
    if (ssl_)
    {
        SSL_free(ssl_);
        ssl_ = nullptr;
        bio_read_ = nullptr;
        bio_write_ = nullptr;
    }
}

bool SsmsDtls::Init()
{
    auto ret = dtls_cert_.Init();
    if (ret < 0)
    {
        LOG_ERROR << "SsmsDtlsCerts Init failed";
        return false;
    }
    ret = InitSSLContext();
    if (!ret)
    {
        LOG_ERROR << "InitSSLContext failed";
        return false;
    }
    ret = InitSSL();

    return true;
}

void SsmsDtls::OnRecv(const SsmsUdpSocketPtr &udp_socket, const SsmsUdpPktPtr &in_pkt)
{
    BIO_reset(bio_read_);
    BIO_reset(bio_write_);

    BIO_write(bio_read_, in_pkt->data_, in_pkt->len_);
    SSL_do_handshake(ssl_);

    NeedPost(udp_socket);

    if (is_done_)
    {
        GetSrtpKey();
        return;
    }
    SSL_read(ssl_, buffer_, sizeof(buffer_));
}

const std::string &SsmsDtls::Fingerprint() const
{
    return dtls_cert_.Fingerprint();
}

void SsmsDtls::SetDone()
{
    is_done_ = true;
}

void SsmsDtls::SetClient(bool client)
{
    is_client_ = true;
}

const std::string &SsmsDtls::SendKey()
{
    return send_key_;
}

const std::string &SsmsDtls::RecvKey()
{
    return recv_key_;
}

bool SsmsDtls::InitSSLContext()
{
    ssl_context_ = SSL_CTX_new(DTLS_method());
    SSL_CTX_use_certificate(ssl_context_, dtls_cert_.GetCerts());
    auto ret = SSL_CTX_use_PrivateKey(ssl_context_, dtls_cert_.GetPrivateKey());
    if (!ret)
    {
        LOG_ERROR << "SSL_CTX_use_PrivateKey failed";
        return false;
    }
    ret = SSL_CTX_check_private_key(ssl_context_);
    if (!ret)
    {
        LOG_ERROR << "SSL_CTX_check_private_key failed";
        return false;
    }
    SSL_CTX_set_cipher_list(ssl_context_, "ALL");
    SSL_CTX_set_verify(ssl_context_, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, SsmsDtls::SSLVerify);
    SSL_CTX_set_info_callback(ssl_context_, SsmsDtls::SSLInfo);

    SSL_CTX_set_verify_depth(ssl_context_, 4);
    SSL_CTX_set_read_ahead(ssl_context_, 1);

    SSL_CTX_set_tlsext_use_srtp(ssl_context_, "SRTP_AES128_CM_SHA1_80");
    return true;
}

bool SsmsDtls::InitSSL()
{
    ssl_ = SSL_new(ssl_context_);
    if (!ssl_)
    {
        LOG_ERROR << "SSL_new failed";
        return false;
    }

    SSL_set_ex_data(ssl_, 0, static_cast<void *>(this));

    bio_read_ = BIO_new(BIO_s_mem());
    bio_write_ = BIO_new(BIO_s_mem());

    SSL_set_bio(ssl_, bio_read_, bio_write_);

    SSL_set_mtu(ssl_, 1350);
    SSL_set_accept_state(ssl_);
    return true;
}

int SsmsDtls::SSLVerify(int preverify_ok, X509_STORE_CTX *ctx)
{
    return 1;
}

void SsmsDtls::SSLInfo(const SSL *ssl, int where, int ret)
{
    SsmsDtls *dtls = static_cast<SsmsDtls *>(SSL_get_ex_data(ssl, 0));
    int w = where & ~SSL_ST_MASK;

    if (w & SSL_ST_CONNECT)
    {
        dtls->SetClient(true);
    }
    else if (w & SSL_ST_ACCEPT)
    {
        dtls->SetClient(false);
    }
    else
    {
        dtls->SetClient(false);
    }

    if (where & SSL_CB_HANDSHAKE_DONE)
    {
        dtls->SetDone();
        LOG_DEBUG << "dtls handshake done";
    }
}

void SsmsDtls::NeedPost(const SsmsUdpSocketPtr &udp_socket)
{
    if (BIO_eof(bio_write_))
    {
        return;
    }

    char *data = nullptr;
    auto read = BIO_get_mem_data(bio_write_, &data);
    if (read <= 0)
    {
        return;
    }

    SsmsUdpPktPtr out_pkt = std::make_shared<SsmsUdpPkt>(data, read);
    udp_socket->SendPacket(out_pkt);

    BIO_reset(bio_write_);
}

void SsmsDtls::GetSrtpKey()
{
    int32_t srtp_key_len = 16;
    int32_t srtp_salt_len = 14;

    unsigned char material[30 * 2];
    static std::string label = "EXTRACTOR-dtls-srtp";
    auto ret = SSL_export_keying_material(ssl_, material, sizeof(material),
                                        label.c_str(), label.size(), NULL, 0, 0);
    if (ret <= 0)    
    {
        LOG_ERROR << "SSL_export_keying_material failed";
    }

    int32_t offset = 0;
    std::string client_master_key((char *)material, srtp_key_len);
    offset += srtp_key_len;
    std::string server_master_key((char *)(material + offset), srtp_key_len);
    offset += srtp_key_len;
    std::string client_master_salt((char *)(material + offset), srtp_salt_len);
    offset += srtp_salt_len;
    std::string server_master_salt((char *)(material + offset), srtp_salt_len);
    offset += srtp_salt_len;

    if (is_client_)
    {
        recv_key_ = server_master_key + server_master_salt;
        send_key_ = client_master_key + client_master_salt;
    }
    else
    {
        recv_key_ = client_master_key + client_master_salt;
        send_key_ = server_master_key + server_master_salt;
    }
}