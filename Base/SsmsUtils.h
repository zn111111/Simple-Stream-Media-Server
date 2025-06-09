#pragma once

#include <stdint.h>
#include "NonCopyable.h"

namespace ssms
{
    namespace base
    {
        class SsmsUtils : public NonCopyable
        {
        public:
            SsmsUtils() = delete;
            ~SsmsUtils() = delete;

            static void Write1Byte(char *data, uint8_t src);
            static void Write2BytesBe(char *data, uint16_t src);
            static void Write3BytesBe(char *data, uint32_t src);
            static void Write4BytesLe(char *data, uint32_t src);
            static void Write4BytesBe(char *data, uint32_t src);
            static bool Compare(double d1, double d2, double eps = 1e-9);
        };
    }
}