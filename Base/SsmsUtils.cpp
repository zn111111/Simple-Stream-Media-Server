#include <string.h>
#include <arpa/inet.h>
#include <math.h>
#include "SsmsUtils.h"

using namespace ssms::base;

static uint32_t __crc32_MPEG_table[256];
static bool __crc32_MPEG_table_initialized = false;

static uint32_t __crc32_IEEE_table[256];
static bool __crc32_IEEE_table_initialized = false;

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

void SsmsUtils::WriteNBytes(char *data, const char *src, int len)
{
    memcpy(data, src, len);
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

std::vector<std::string> SsmsUtils::Split(const std::string &src, const std::string &flag)
{
    int begin = 0;
    size_t end = std::string::npos;
    std::vector<std::string> v;
    while ((end = src.find(flag, begin)) != std::string::npos)
    {
        v.emplace_back(src.substr(begin, end - begin).c_str());
        begin = end + flag.size();
    }

    if (begin < src.size())
    {
        v.emplace_back(src.substr(begin).c_str());
    }

    return v;
}

uint64_t __crc32_reflect(uint64_t data, int width)
{
    uint64_t res = data & 0x01;
    
    for (int i = 0; i < (int)width - 1; i++) {
        data >>= 1;
        res = (res << 1) | (data & 0x01);
    }
    
    return res;
}

void __crc32_make_table(uint32_t t[256], uint32_t poly, bool reflect_in)
{
    int width = 32; // 32bits checksum.
    uint64_t msb_mask = (uint32_t)(0x01 << (width - 1));
    uint64_t mask = (uint32_t)(((msb_mask - 1) << 1) | 1);
    
    int tbl_idx_width = 8; // table index size.
    int tbl_width = 0x01 << tbl_idx_width; // table size: 256
    
    for (int i = 0; i < (int)tbl_width; i++) {
        uint64_t reg = uint64_t(i);
        
        if (reflect_in) {
            reg = __crc32_reflect(reg, tbl_idx_width);
        }
        
        reg = reg << (width - tbl_idx_width);
        for (int j = 0; j < tbl_idx_width; j++) {
            if ((reg&msb_mask) != 0) {
                reg = (reg << 1) ^ poly;
            } else {
                reg = reg << 1;
            }
        }
        
        if (reflect_in) {
            reg = __crc32_reflect(reg, width);
        }
        
        t[i] = (uint32_t)(reg & mask);
    }
}

void SsmsUtils::InitCrc32()
{
    uint32_t poly = 0x04C11DB7;
    bool reflect_in = false;
    if (!__crc32_MPEG_table_initialized)
    {
        __crc32_make_table(__crc32_MPEG_table, poly, reflect_in);
        __crc32_MPEG_table_initialized = true;
    }

    reflect_in = true;
    if (!__crc32_IEEE_table_initialized)
    {
        __crc32_make_table(__crc32_IEEE_table, poly, reflect_in);
        __crc32_IEEE_table_initialized = true;
    }
}

uint32_t __crc32_table_driven(uint32_t* t, const void* buf, int size, uint32_t previous, bool reflect_in, uint32_t xor_in, bool reflect_out, uint32_t xor_out)
{
    int width = 32; // 32bits checksum.
    uint64_t msb_mask = (uint32_t)(0x01 << (width - 1));
    uint64_t mask = (uint32_t)(((msb_mask - 1) << 1) | 1);
    
    int tbl_idx_width = 8; // table index size.
    
    uint8_t* p = (uint8_t*)buf;
    uint64_t reg = 0;
    
    if (!reflect_in) {
        reg = xor_in;
        
        for (int i = 0; i < (int)size; i++) {
            uint8_t tblidx = (uint8_t)((reg >> (width - tbl_idx_width)) ^ p[i]);
            reg = t[tblidx] ^ (reg << tbl_idx_width);
        }
    } else {
        reg = previous ^ __crc32_reflect(xor_in, width);
        
        for (int i = 0; i < (int)size; i++) {
            uint8_t tblidx = (uint8_t)(reg ^ p[i]);
            reg = t[tblidx] ^ (reg >> tbl_idx_width);
        }
        
        reg = __crc32_reflect(reg, width);
    }
    
    if (reflect_out) {
        reg = __crc32_reflect(reg, width);
    }
    
    reg ^= xor_out;
    return (uint32_t)(reg & mask);
}

uint32_t SsmsUtils::Crc32Mpegts(const char* buf, int size)
{
    bool reflect_in = false;
    uint32_t xor_in = 0xffffffff;
    bool reflect_out = false;
    uint32_t xor_out = 0x0;
    
    return __crc32_table_driven(__crc32_MPEG_table, buf, size, 0x00, reflect_in, xor_in, reflect_out, xor_out);
}

uint32_t SsmsUtils::Crc32Ieee(const char* buf, int size, uint32_t previous)
{   
    bool reflect_in = true;
    uint32_t xor_in = 0xffffffff;
    bool reflect_out = true;
    uint32_t xor_out = 0xffffffff;
    return __crc32_table_driven(__crc32_IEEE_table, buf, size, previous, reflect_in, xor_in, reflect_out, xor_out);
}