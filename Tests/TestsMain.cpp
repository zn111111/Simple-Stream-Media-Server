#include <iostream>
#include "SsmsConfigTest.h"
#include "SsmsLogStreamTest.h"
#include "Base/SsmsINIReader.h"
#include "SsmsEventLoopThreadTest.h"
#include "SsmsAcceptorTest.h"
#include "SsmsThreadLoopReadWriteTest.h"
#include "SsmsTcpServerTest.h"
#include "SsmsRtmpHandshakeTest.h"
#include "SsmsTimingWheelTest.h"
#include "SsmsUdpServerTest.h"
#include "Base/SsmsUtils.h"

int main()
{
    S_SSMSCONFIG->LoadConfigFile("../../Resource/Ssms.conf");
    if (S_SSMSCONFIG->ParseError() != 0)
    {
        std::cout << "load config error" << std::endl;
        return -1;
    }

    SsmsUtils::InitCrc32();

    // TestConfig();
    // TestLog();
    // EventLoopThreadTest();
    // TestAcceptor();
    // TestRead();
    // TestTcpServer();
    // RtmpHandshakeTest();
    // TestTimingWheel();
    TestUdpServer();

    return 0;
}