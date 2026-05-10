#pragma once

#include <cstdint>
#include <string>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include "SsmsDtlsCerts.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsDtls
        {
        public:
            SsmsDtls();
            ~SsmsDtls();

            bool Init();
            void OnRecv(const SsmsUdpSocketPtr &udp_socket, const SsmsUdpPktPtr &in_pkt);
            const std::string &Fingerprint() const;
            void SetDone();
            void SetClient(bool client);
            const std::string &SendKey();
            const std::string &RecvKey();
        private:
            bool InitSSLContext();
            bool InitSSL();
            static int SSLVerify(int preverify_ok, X509_STORE_CTX *ctx);
            static void SSLInfo(const SSL *ssl, int where, int ret);
            void NeedPost(const SsmsUdpSocketPtr &udp_socket);
            void GetSrtpKey();

            SSL_CTX *ssl_context_{nullptr};
            SsmsDtlsCerts dtls_cert_;
            bool is_client_{false};
            bool is_done_{false};
            SSL *ssl_{nullptr};
            BIO *bio_read_{nullptr};
            BIO *bio_write_{nullptr};
            char buffer_[65535]{0};
            std::string send_key_;
            std::string recv_key_;
        };
    }
}