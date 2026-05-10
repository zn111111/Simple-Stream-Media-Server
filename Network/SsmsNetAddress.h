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
            uint16_t GetPort() const ;
            static void GetIpPort(const struct sockaddr *addr, bool ipv4, std::string &ip, int &port);
            bool Ipv4() const;
            std::string GetFourTuple() const;
        private:
            bool ipv4_{false};
            std::string ip_;
            uint16_t port_;
        };
    }
}