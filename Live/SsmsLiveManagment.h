#pragma once

#include <mutex>
#include <unordered_map>
#include <string>
#include "Base/NonCopyable.h"
#include "SsmsLiveDefine.h"

using namespace ssms::base;

namespace ssms
{
    namespace live
    {
        //负责管理整个直播业务, 运行在主线程
        class SsmsLiveManagment : public NonCopyable
        {
        public:
            SsmsLiveManagment() = default;
            ~SsmsLiveManagment() = default;

            SsmsSessionPtr CreateSession(const std::string &stream);
            SsmsSessionPtr GetSession(const std::string &stream);
            void DeleteSession(const std::string &stream);
            bool Exist(const std::string &stream);
        private:
            std::mutex lock_;
            //app/stream : SsmsSessionPtr
            std::unordered_map<std::string, SsmsSessionPtr> sessions_;
        };
    }
}