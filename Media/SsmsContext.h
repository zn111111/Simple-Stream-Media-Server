#pragma once

#include <memory>
#include <list>
#include "Base/SsmsBuffer.h"
#include "SsmsMediaDefine.h"
#include "Network/SsmsNetworkDefine.h"

using namespace ssms::base;
using namespace ssms::nw;

namespace ssms
{
    namespace media
    {
        class SsmsContext : public std::enable_shared_from_this<SsmsContext>
        {
            friend class ssms::nw::TcpConnection;
        public:
            SsmsContext();
            virtual ~SsmsContext();

            virtual int Parse(const SsmsBufferPtr &data) = 0;
            virtual void ClearSendCompleteData();
            template <typename T>
            std::shared_ptr<T> GetContext() const
            {
                return std::dynamic_pointer_cast<T>(shared_from_this());
            }
        protected:
            SsmsClientPtr client_;
            SsmsSessionPtr sess_;
            SsmsLiveManagmentPtr live_manage_;
            std::string app_;
            std::string stream_;
        };
    }
}