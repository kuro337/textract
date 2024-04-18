#include <gtest/gtest.h>
#include <string>
#include <textract.h>
#include <vector>

class PublicAPITests : public ::testing::Test {
  public:
    const std::string inputFile = "../../images/imgtext.jpeg";

    const std::string outputInNewFolderInInputImages = "../../images/processed";

    const std::string path = "../../images/";
    const std::vector<std::string> images = {
        path + "screenshot.png", path + "imgtext.jpeg",
        path + "compleximgtext.png", path + "scatteredtext.png"};

    const std::string outputDirWrite = "tmpapi";
    const std::string diroutputTest = "tmpdir";

    imgstr::ImgProcessor app;

  protected:
    void SetUp() override {}

    void TearDown() override {

        std::filesystem::remove_all(outputDirWrite);
        std::filesystem::remove_all(diroutputTest);
        std::filesystem::remove_all(outputInNewFolderInInputImages);
    }
};

/* All Tests Passed Memory Sanitization ASan */

TEST_F(PublicAPITests, GetTextFromOneImage) {

    auto text = app.getImageText(inputFile);
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ("HAWAII\n", text.value());

    if (text) {
        EXPECT_EQ("HAWAII\n", text.value());
    }
}

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

TEST_F(PublicAPITests, ProcessSimpleDir) {

    EXPECT_NO_THROW(app.simpleProcessDir(path, outputDirWrite););
}

TEST_F(PublicAPITests, ProcessFilesFromDir) {

    EXPECT_NO_THROW(
        app.processImagesDir(path, true, outputInNewFolderInInputImages));
}

TEST_F(PublicAPITests, Results) { EXPECT_NO_THROW(app.getResults()); }

/*

Document Mode : 3927 ms
Image    Mode : 4120 ms

ImgMode::image sets : ocrPtr->SetPageSegMode(tesseract::PSM_AUTO);

*/

TEST_F(PublicAPITests, AddImagesThenConvertToTextDocumentMode) {

    app.addFiles(images);

    EXPECT_NO_THROW(app.convertImagesToTextFiles(outputDirWrite));
};

TEST_F(PublicAPITests, AddImagesThenConvertToTextImageMode) {

    app.setImageMode(imgstr::ImgMode::image);

    app.addFiles(images);

    EXPECT_NO_THROW(app.convertImagesToTextFiles(outputDirWrite));

    app.getResults();
};
