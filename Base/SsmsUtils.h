#pragma once

#include <string>
#include <stdint.h>
#include <vector>
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
            //写入n个独立字节的数据, 不是n个字节组成的整型数据时使用
            static void WriteNBytes(char *data, const char *src, int len);
            static bool Compare(double d1, double d2, double eps = 1e-9);
            //返回filepath的上级目录加上当前文件, dir/file
            static std::string GetParentDirWithFilename(const std::string &filepath);
            //将src以flag为分隔符拆分
            static std::vector<std::string> Split(const std::string &src, const std::string &flag);

            static void InitCrc32();
            //用于MPEG-TS里CRC的计算
            static uint32_t Crc32Mpegts(const char* buf, int size);
            //通用的标准CRC-32实现
            static uint32_t Crc32Ieee(const char* buf, int size, uint32_t previous = 0);
        };
    }
}