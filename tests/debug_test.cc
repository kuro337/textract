
#include <gtest/gtest.h>
#include <util.h>

class DebugTest: public ::testing::Test {};

using logging::LogLevel::Err;
using logging::LogLevel::Info;

TEST_F(DebugTest, BasicAssertions) {
    Debug<Err>("An error occurred: {0}", "Some Error");

    Debug<Info, ThreadLocalConfig>("ThreadLocal Destructor Called");

    Debug<Err, CacheConfig>("This is an informational message with "
                            "CacheConfig");

    Debug<Info, MutexConfig>("Mutex Log Disabled in Build");

    Debug<Info>("This is a default informational message");
}

auto main(int argc, char **argv) -> int {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
