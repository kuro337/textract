#include <fs.h>
#include <gtest/gtest.h>
#include <util.h>

namespace const_tests_constants {
    static constexpr auto tempDirectory = "temporary_dir";

} // namespace const_tests_constants

using namespace const_tests_constants;

class ConstTests: public ::testing::Test {
  protected:
    // static std::vector<std::string> static_memb;
    void SetUp() override {
        // runs per test
    }
    void TearDown() override {
        // runs per test
    }

    static void SetUpTestSuite() {
        // Once for whole Fixture
        // static_member = initializeStaticMember();
    }

    static void TearDownTestSuite() {
        // Once for Whole Fixture
    }

  public:
    std::string tempDir = "temp_dir";
};

TEST_F(ConstTests, BasicAssertions) {}

TEST_F(ConstTests, BasicTest) {
    std::vector<std::pair<std::string, bool>> test = {{"validimage.png", true},
                                                      {"VALID.PNG", true},
                                                      {"valid..jpeg", true},
                                                      {"invalidnoend", false},
                                                      {"invalid.nan", false},
                                                      {"..", false},
                                                      {"notvalidextension.txt", false},
                                                      {"invalidnoend", false},
                                                      {"", false}};

    for (const auto &[filename, expected]: test) {
        bool actual = isImageFile(filename);
        EXPECT_EQ(expected, actual) << "Failed for filename: " << filename;
    }
}

auto main(int argc, char **argv) -> int {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
