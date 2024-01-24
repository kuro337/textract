#include <gtest/gtest.h>
#include <string>
#include <textract.h>
#include <vector>

class PublicAPITests : public ::testing::Test {
public:
  const std::string inputFile = "../../../images/imgtext.jpeg";

  const std::string outputInNewFolderInInputImages =
      "../../../images/processed";

  const std::string path = "../../../images/";
  const std::vector<std::string> images = {
      path + "screenshot.png", path + "imgtext.jpeg",
      path + "compleximgtext.png", path + "scatteredtext.png"};

  const std::string outputDirWrite = "tmpapi";
  const std::string diroutputTest = "tmpdir";

  imgstr::ImgProcessor app = imgstr::ImgProcessor();

protected:
  void SetUp() override {}

  void TearDown() override {

    std::filesystem::remove_all(outputDirWrite);
    std::filesystem::remove_all(diroutputTest);
    std::filesystem::remove_all(outputInNewFolderInInputImages);
  }
};

TEST_F(PublicAPITests, GetTextFromOneImage) {

  auto text = app.getImageText(inputFile);
  ASSERT_TRUE(text.has_value()); // Ensure text is not std::nullopt
  EXPECT_EQ("HAWAII\n", text.value());

  if (text) {
    EXPECT_EQ("HAWAII\n", text.value());
  }
}

TEST_F(PublicAPITests, ProcessFilesFromDir) {

  app.processImagesDir(path, true, outputInNewFolderInInputImages);
  EXPECT_NO_THROW(app.getResults());
}

TEST_F(PublicAPITests, Results) { EXPECT_NO_THROW(app.getResults()); }

TEST_F(PublicAPITests, AddImagesThenConvertToTextDocumentMode) {

  app.addFiles(images);

  EXPECT_NO_THROW(app.convertImagesToTextFiles(outputDirWrite));
};

TEST_F(PublicAPITests, AddImagesThenConvertToTextImageMode) {

  app.setImageMode(imgstr::ImgMode::image);

  app.addFiles(images);

  EXPECT_NO_THROW(app.convertImagesToTextFiles(outputDirWrite));

  // 0.57019 ms with   api->SetPageSegMode(tesseract::PSM_AUTO);
  // 0.902931   with   default Page Mode
};

/*

without openmp :

4 Files Processed and Converted in 1.61664 seconds
4 Files Processed and Converted in 1.62264 seconds


with openmp

4 Files Processed and Converted in 1.19036 seconds
4 Files Processed and Converted in 1.16097 seconds

*/
