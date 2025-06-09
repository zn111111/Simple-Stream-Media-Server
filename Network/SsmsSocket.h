#pragma once

#include "Base/NonCopyable.h"

using namespace ssms::base;

namespace ssms
{
    namespace nw
    {
        class SsmsSocket : public NonCopyable
        {
        public:
            SsmsSocket() = delete;
            ~SsmsSocket() = delete;

            static int CreateTcpNonBlockSocket();
            static int CreateUdpNonBlockSocket();
            static void Bind(int fd, const struct sockaddr &addr);
            static void Listen(int fd);
            static void SetAddrReuse(int fd);
            static void SetPortReuse(int fd);
        };
    }
}