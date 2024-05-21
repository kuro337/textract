#include <fs.h>
#include <gtest/gtest.h>
#include <string>
#include <textract.h>
#include <vector>

/* Test Processes and Converts all images in /images
Stores results in -> /Users/kuro/Documents/Code/Cpp/image/imgapp/build/tests/processed */
namespace {
    constexpr auto *const imgFolder     = IMAGE_FOLDER_PATH;
    constexpr auto *const inputOpenTest = INPUT_OPEN_TEST_PATH;
} // namespace
class PublicAPITests: public ::testing::Test {
  public:
    PublicAPITests() { fpaths = getFilePaths(imgFolder).get(); }

    std::vector<std::string> fpaths;

    const std::string tempDir = "processed";

    std::unique_ptr<imgstr::ImgProcessor> app = std::make_unique<imgstr::ImgProcessor>();

  protected:
    void SetUp() override {}

    void TearDown() override {
        if (deleteDirectories(tempDir)) {
            FAIL() << "Failed to Cleanup Temp Dirs\n";
        }
    }
};

/* All Tests Passed Memory Sanitization ASan */

TEST_F(PublicAPITests, GetTextFromOneImage) {
    //    auto text = app.getImageText(fpaths[0]);
    auto text = app->getImageText(fpaths[0]);
    ASSERT_TRUE(text.has_value());
}

TEST_F(PublicAPITests, ProcessSimpleDir) {
    ///    app.setCores(4);
    app->setCores(4);

    EXPECT_NO_THROW(app->simpleProcessDir(imgFolder, tempDir););
}

TEST_F(PublicAPITests, ProcessFilesFromDir) {
    EXPECT_NO_THROW(app->processImagesDir(imgFolder, true, tempDir));
}

TEST_F(PublicAPITests, Results) { EXPECT_NO_THROW(app->getResults()); }

/*

Performance Comparison Table

ProcessFilesFromDirCacheImpl
----------------------------------------
| Environment | Mode           | Average Time (ms) | Total Time (seconds) |
|-------------|----------------|-------------------|----------------------|
| Linux       | 4 Cores        | 1.5921            | 5.72182              |
| Linux       | Single Threaded| 1.2733            | 10.1879              |
| Mac         | 4 Cores        | 0.712196          | 3.34019              |
| Mac         | Single Threaded| 0.650348          | 5.23831              |
---------------------------------------------------------------------------

ProcessFilesFromDirSimpleImpl
-------------------------------------------------------------------------------
| Environment | Mode           | Average Time (ms) | Total Time (seconds) |
|-------------|----------------|-------------------|----------------------|
| Linux       | 4 Cores        | 1.5921            | 5.72182              |
| Linux       | Single Threaded| 1.2733            | 10.1879              |
| Mac         | 4 Cores        | 0.712196          | 3.34019              |
| Mac         | Single Threaded| 0.650348          | 5.23831              |
---------------------------------------------------------------------------


Comparison of Overhead from Atomic Caching Logic:

-------------------------------------------------------------------------
| Processing Type | Execution Time 1 (ms) | Execution Time 2 (ms) | ... |
|-----------------|-----------------------|-----------------------|-----|
| Simple          | 291                   | 5790                  | ... |
| Parallel        | 317                   | 6082                  | ... |
| Simple          | 35                    | 393                   | ... |
| Parallel        | 36                    | 406                   | ... |
| Simple          | 2719                  | 982                   | ... |
| Parallel        | 2802                  | 1005                  | ... |
| Simple          | 12                    |                       |     |
| Parallel        | 12                    |                       |     |
-------------------------------------------------------------------------

Note: The times are measured in milliseconds. The comparison is based on
execution times for different tasks or iterations within the 'Simple' and
'Parallel' approaches.


*/

/*

Document Mode : 3927 ms
Image    Mode : 4120 ms

ImgMode::image sets : ocrPtr->SetPageSegMode(tesseract::PSM_AUTO);

*/

TEST_F(PublicAPITests, AddImagesThenConvertToTextDocumentMode) {
    app->addFiles(fpaths);

    EXPECT_NO_THROW(app->convertImagesToTextFiles(tempDir));
};

TEST_F(PublicAPITests, AddImagesThenConvertToTextImageMode) {
    app->setImageMode(ImgMode::image);

    app->addFiles(fpaths);

    EXPECT_NO_THROW(app->convertImagesToTextFiles(tempDir));

    app->getResults();
};

auto main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
