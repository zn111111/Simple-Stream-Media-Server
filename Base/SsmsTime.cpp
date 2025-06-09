#include <ctime>
#include <sstream>
#include "SsmsTime.h"

using namespace ssms::base;

std::string SsmsTime::CurrTimeString()
{
    time_t tv = time(nullptr);
    tm tm;
    localtime_r(&tv, &tm);
    std::ostringstream stream;
    stream << tm.tm_year + 1900 << "-" << tm.tm_mon + 1 << "-" << tm.tm_mday
    << " " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec;
    return stream.str();
}