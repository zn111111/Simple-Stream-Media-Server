#pragma once

#include <sstream>
#include <cstdint>
#include <thread>
#include "NonCopyable.h"
#include "SsmsINIReader.h"

namespace ssms
{
    namespace base
    {
        enum
        {
            kLogLevelTrace,
            LogLevelDebug,
            kLogLevelInfo,
            LogLevelWarn,
            LogLevelError,
            LogLevelDefault
        };

        class SsmsLogStream : public NonCopyable
        {
        public:
            SsmsLogStream(const std::thread::id &thread_id, const std::string &log_level, const std::string &filename, uint32_t line, const std::string &func = "");
            ~SsmsLogStream();

            template <typename T>
            SsmsLogStream &operator<<(const T &msg)
            {
                stream_ << msg;
                return *this;
            }
        private:
            std::ostringstream stream_;
        };

        #define LOG_TRACE if (S_SSMSCONFIG->GetInteger("LOG", "log_level", LogLevelDefault) <= kLogLevelTrace) \
                            SsmsLogStream(std::this_thread::get_id(), "TRACE", __FILE__, __LINE__, __func__)
        #define LOG_DEBUG if (S_SSMSCONFIG->GetInteger("LOG", "log_level", LogLevelDefault) <= LogLevelDebug) \
                            SsmsLogStream(std::this_thread::get_id(), "DEBUG", __FILE__, __LINE__, __func__)
        #define LOG_INFO if (S_SSMSCONFIG->GetInteger("LOG", "log_level", LogLevelDefault) <= kLogLevelInfo) \
                            SsmsLogStream(std::this_thread::get_id(), "INFO", __FILE__, __LINE__)
        #define LOG_WARN SsmsLogStream(std::this_thread::get_id(), "WARN", __FILE__, __LINE__, __func__)
        #define LOG_ERROR SsmsLogStream(std::this_thread::get_id(), "ERROR", __FILE__, __LINE__, __func__)
    }
}