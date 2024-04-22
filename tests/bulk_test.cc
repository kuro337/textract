#include <gtest/gtest.h>
#include <textract.h>

class PublicAPITests: public ::testing::Test {
    // public:
    //   const std::string outputBulkImageMode = "../../../bulk/image_mode";
    //   const std::string outputBulkDocMode = "../../../bulk/document_mode";

    //   const std::string path = "../../../bulk/";

    //   imgstr::ImgProcessor app = imgstr::ImgProcessor();

    // protected:
    //   void SetUp() override {}

    //   void TearDown() override {
    //       std::filesystem::remove_all(outputBulkImageMode);
    //   }
};

TEST_F(PublicAPITests, Placeholder) { EXPECT_EQ(1 + 1, 2); };

// TEST_F(PublicAPITests, ProcessBulkDocumentsSimple) {

//     app.simpleProcessDir(path, outputBulkDocMode);
// }

// TEST_F(PublicAPITests, ProcessBulkDocumentImages) {

//     app.processImagesDir(path, true, outputBulkDocMode);
//     EXPECT_NO_THROW(app.getResults());

//     /*

//         1000 Files
//         Total Time      : 680387 ms ~ 11 minutes
//         Average Latency : 0.679933 ms

//     */
// }

// TEST_F(PublicAPITests, ProcessFilesImageMode) {
//     app.setImageMode(imgstr::ImgMode::image);
//     app.processImagesDir(path, true, outputBulkImageMode);

//     EXPECT_NO_THROW(app.getResults());
// }

// TEST_F(PublicAPITests, ProcessFilesDocumentMode) {
//     app.setImageMode(imgstr::ImgMode::document);
//     app.processImagesDir(path, true, outputBulkDocMode);
//     EXPECT_NO_THROW(app.getResults());
// }

/*

1000 images : 20s

Avg Latency : 0.280493 ms

*/