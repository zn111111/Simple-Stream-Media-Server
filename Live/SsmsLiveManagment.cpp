#include "SsmsLiveManagment.h"
#include "SsmsSession.h"
#include "Base/SsmsLogStream.h"

using namespace ssms::live;

SsmsSessionPtr SsmsLiveManagment::CreateSession(const std::string &stream)
{
    std::lock_guard<std::mutex> lk(lock_);
    if (sessions_.find(stream) != sessions_.end())
    {
        return sessions_[stream];
    }

    LOG_DEBUG << "create stream " << stream << " session";
    SsmsSessionPtr session = std::make_shared<SsmsSession>();
    sessions_[stream] = session;

    return session;
}

SsmsSessionPtr SsmsLiveManagment::GetSession(const std::string &stream)
{
    std::lock_guard<std::mutex> lk(lock_);
    if (sessions_.find(stream) != sessions_.end())
    {
        return sessions_[stream];
    }

    return std::shared_ptr<SsmsSession>();
}

void SsmsLiveManagment::DeleteSession(const std::string &stream)
{
    std::lock_guard<std::mutex> lk(lock_);
    if (sessions_.find(stream) != sessions_.end())
    {
        sessions_.erase(stream);
    }
}

bool SsmsLiveManagment::Exist(const std::string &stream)
{
    std::lock_guard<std::mutex> lk(lock_);
    return sessions_.find(stream) != sessions_.end();
}