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
            static bool Compare(double d1, double d2, double eps = 1e-9);
            //返回filepath的上级目录加上当前文件, dir/file
            static std::string GetParentDirWithFilename(const std::string &filepath);
            //将src以flag为分隔符拆分
            static std::vector<int> Split(const std::string &src, const std::string &flag);
        };
    }
}