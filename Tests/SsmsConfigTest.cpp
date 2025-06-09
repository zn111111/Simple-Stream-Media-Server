#include <iostream>
#include "SsmsConfigTest.h"
#include "Base/SsmsINIReader.h"

using namespace ssms::base;

void TestConfig()
{
    int max_store_time = S_SSMSCONFIG->GetInteger("COMMON", "max_store_time", 0);
    std::cout << "max_store_time: " << max_store_time << std::endl;
}