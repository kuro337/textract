#include <conversion.h>
#include <generated_files.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <textract.h>
namespace {
    constinit auto *const imgFolder = IMAGE_FOLDER_PATH;
}

class ParallelPerfTesseract: public ::testing::Test {
  public:
    imgstr::ImgProcessor imageTranslator;

  protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(ParallelPerfTesseract, OEMvsLSTMAnalysisDebug) {
    ASSERT_NO_THROW(convertImagesToText(fileNames));
}

auto main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
