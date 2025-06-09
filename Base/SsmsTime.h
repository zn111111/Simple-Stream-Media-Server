#pragma once

#include "NonCopyable.h"

namespace ssms
{
    namespace base
    {
        class SsmsTime : public NonCopyable
        {
        public:
            SsmsTime() = delete;
            ~SsmsTime() = delete;

            static std::string CurrTimeString();
        };
    }
}