#pragma once

#include "Media/SsmsContext.h"
#include "Network/SsmsTcpConnection.h"

using namespace ssms::media;
using namespace ssms::nw;

void TestRead();

class HttpContextTest : public SsmsContext
{
public:
    HttpContextTest(const TcpConnectionPtr &conn);
    ~HttpContextTest() = default;

    int Parse(const SsmsBufferPtr &data) override;
private:
    std::weak_ptr<TcpConnection> conn_;
};