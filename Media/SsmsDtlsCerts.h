#pragma once

#include <stdint.h>
#include <string>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

namespace ssms
{
    namespace media
    {
        class SsmsDtlsCerts
        {
        public:
            SsmsDtlsCerts() = default;
            ~SsmsDtlsCerts();

            int Init();
            const std::string &Fingerprint() const;
            EVP_PKEY *GetPrivateKey();
            X509 *GetCerts() const;
            uint32_t GenRandom();
        private:
            EVP_PKEY *dtls_pkey_{nullptr};
            X509 *dtls_certs_{nullptr};
            std::string fingerprint_;
        };
    }
}