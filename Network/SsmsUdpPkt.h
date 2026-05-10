#pragma once

#include <cstring>

namespace ssms
{
    namespace nw
    {
        struct SsmsUdpPkt
        {
            SsmsUdpPkt(int len)
            {
                data_ = new char[len];
                memset(data_, 0, len);
                capacity_ = len;
            }
            SsmsUdpPkt(char *data, int len)
            {
                data_ = new char[len];
                memcpy(data_, data, len);
                len_ = len;
                capacity_ = len;
            }
            ~SsmsUdpPkt()
            {
                delete[] data_;
                data_ = nullptr;
                len_ = -1;
                capacity_ = -1;
            }

            char *data_{nullptr};
            int len_{-1};
            int capacity_{-1};
        };
    }
}