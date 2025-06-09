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
        protected:
            SsmsClientPtr client_;
            SsmsSessionPtr sess_;
            SsmsLiveManagmentPtr live_manage_;
            std::string app_;
            std::string stream_;
        };
    }
}