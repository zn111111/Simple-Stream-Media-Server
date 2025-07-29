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

std::string SsmsUtils::GetParentDirWithFilename(const std::string &filepath)
{
    int pos = filepath.size() - 1;
    int end_pos = filepath.size() - 1;
    if (filepath.empty())
    {
        return filepath;
    }
    else if ('/' == filepath[pos])
    {
        pos--;
        end_pos--;
    }

    if ((pos = filepath.rfind('/', pos)) == std::string::npos || (pos = filepath.rfind('/', pos - 1)) == std::string::npos)
    {
        return filepath;
    }

    return filepath.substr(pos + 1, end_pos - pos);
}

std::vector<int> SsmsUtils::Split(const std::string &src, const std::string &flag)
{
    int begin = 0;
    size_t end = std::string::npos;
    std::vector<int> v;
    while ((end = src.find(flag, begin)) != std::string::npos)
    {
        v.emplace_back(atoi(src.substr(begin, end - begin).c_str()));
        begin = end + 1;
    }

    if (begin < src.size())
    {
        v.emplace_back(atoi(src.substr(begin).c_str()));
    }

    return v;
}