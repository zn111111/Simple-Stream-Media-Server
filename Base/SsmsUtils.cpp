#include <string.h>
#include <arpa/inet.h>
#include <math.h>
#include "SsmsUtils.h"

using namespace ssms::base;

void SsmsUtils::Write1Byte(char *data, uint8_t src)
{
    *data = src;
}

void SsmsUtils::Write2BytesBe(char *data, uint16_t src)
{
    src = ::htons(src);
    memcpy(data, &src, 2);
}

void SsmsUtils::Write3BytesBe(char *data, uint32_t src)
{
    src = ::htonl(src);
    memcpy(data, (char *)&src + 1, 3);
}

void SsmsUtils::Write4BytesLe(char *data, uint32_t src)
{
    memcpy(data, &src, 4);
}

void SsmsUtils::Write4BytesBe(char *data, uint32_t src)
{
    src = ::htonl(src);
    memcpy(data, &src, 4);
}

bool SsmsUtils::Compare(double d1, double d2, double eps)
{
    return (std::abs(d1 - d2) < eps);
}