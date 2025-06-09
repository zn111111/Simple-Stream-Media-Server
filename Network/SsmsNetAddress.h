#pragma once

#include <string>
#include <memory>

namespace ssms
{
    namespace nw
    {
        class SsmsNetAddress
        {
        public:
            SsmsNetAddress() = default;
            SsmsNetAddress(const std::string &ip, int port, bool ipv4);
            ~SsmsNetAddress() = default;

            void SetIp(const std::string &ip, int port, bool ipv4);
            void GetSockAddr(struct sockaddr *addr);
            std::string GetStringIp() const;
            std::string GetPort() const ;
        private:
            bool ipv4_{false};
            std::string ip_;
            std::string port_;
        };
    }
}