#pragma once

#include <memory>

namespace ssms
{
    namespace live
    {
        class SsmsLiveManagment;
        using SsmsLiveManagmentPtr = std::shared_ptr<SsmsLiveManagment>;

        class SsmsSession;
        using SsmsSessionPtr = std::shared_ptr<SsmsSession>;

        enum SsmsServerProtocol
        {
            SsmsServerProtocolDefault,
            SsmsServerProtocolRTMP
        };
    }
}