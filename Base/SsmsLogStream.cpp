#include <iostream>
#include "SsmsLogStream.h"
#include "SsmsTime.h"
#include "SsmsUtils.h"

using namespace ssms::base;

SsmsLogStream::SsmsLogStream(const std::thread::id &thread_id, const std::string &log_level, const std::string &filename, uint32_t line, const std::string &func)
{
    stream_ << SsmsTime::CurrTimeString() << " " << thread_id << " " << log_level << " [" << SsmsUtils::GetParentDirWithFilename(filename) << ":" << line
    << "]";
    if (!func.empty())
    {
        stream_ << "[" << func << "]";
    }
}

SsmsLogStream::~SsmsLogStream()
{
    stream_ << "\n";
    std::cout << stream_.str();
}