
#include "util.h"
#include <fs.h>
#include <gtest/gtest.h>
#include <textract.h>

namespace imgclass_test_constants {
    static constexpr auto tempDirectory = "temporary_dir";
    constexpr auto *const imgFolder     = IMAGE_FOLDER_PATH;
    constexpr auto *const inputOpenTest = INPUT_OPEN_TEST_PATH;
} // namespace imgclass_test_constants

using namespace imgclass_test_constants;

class Imgclasstest: public ::testing::Test {
  protected:
    // static std::vector<std::string> static_memb;
    static std::vector<std::string>              fpaths;
    static std::unique_ptr<imgstr::ImgProcessor> app;

    void SetUp() override {
        // runs per test
    }
    void TearDown() override {
        // runs per test
    }

    static void SetUpTestSuite() {
        // Once for whole Fixture

        app = std::make_unique<imgstr::ImgProcessor>(1000); // Adjust parameters as needed

        // static_member = initializeStaticMember();
        fpaths = getFilePaths(imgFolder).get();
    }

    static void TearDownTestSuite() {
        // Once for Whole Fixture
    }

  public:
    std::string tempDir = "tempImgClass";
};

// Definition of static members
std::vector<std::string>              Imgclasstest::fpaths;
std::unique_ptr<imgstr::ImgProcessor> Imgclasstest::app;

TEST_F(Imgclasstest, BasicAssertions) {
    // implement test

    ASSERT_FALSE(fpaths.empty()); // Ensure there is at least one path to work with
    if (!fpaths.empty()) {
        ASSERT_NO_THROW(app->convertImageToTextFile(fpaths[0], tempDir));
    }

    ASSERT_TRUE(HandleError<StdErr>(deleteDirectory(tempDir)));
}

auto main(int argc, char **argv) -> int {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
