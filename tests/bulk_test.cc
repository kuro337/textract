#include <gtest/gtest.h>
#include <string>
#include <textract.h>
#include <vector>

class PublicAPITests : public ::testing::Test {
public:
  const std::string outputBulkImageMode = "../../../bulk/image_mode";
  const std::string outputBulkDocMode = "../../../bulk/document_mode";

  const std::string path = "../../../bulk/";

  imgstr::ImgProcessor app = imgstr::ImgProcessor();

protected:
  void SetUp() override {}

  void TearDown() override {

    //  std::filesystem::remove_all(outputInNewFolderInInputImages);
  }
};

TEST_F(PublicAPITests, ProcessFilesImageMode) {
  app.setImageMode(imgstr::ImgMode::image);

  app.processImagesDir(path, true, outputBulkImageMode);
  //   EXPECT_NO_THROW(app.getResults());
}

TEST_F(PublicAPITests, ProcessFilesDocumentMode) {
  // app.setImageMode(imgstr::ImgMode::document);
  //  app.processImagesDir(path, true, outputBulkDocMode);
  //  EXPECT_NO_THROW(app.getResults());
}

/*

1000 images : 20s

Avg Latency : 0.280493 ms

*/