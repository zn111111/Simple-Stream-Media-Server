#include "Base/SsmsLogStream.h"

void TestLog()
{
    LOG_TRACE << "trace log info";
    LOG_DEBUG << "debug log info";
    LOG_INFO << "info log info";
    LOG_WARN << "warn log info";
    LOG_ERROR << "error log info";
}