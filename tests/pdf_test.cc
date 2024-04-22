#include "fs.h"
#include <conversion.h>
#include <gtest/gtest.h>
#include <leptonica/allheaders.h>
#include <llvm/Support/raw_ostream.h>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>

namespace {
    constinit auto *const inputOpenTest = INPUT_OPEN_TEST_PATH;
    const std::string     pdfOutputPath = "my_first_tesseract";

    constexpr auto tessdata_path = "/opt/homebrew/opt/tesseract/share/tessdata";

} // namespace
class PDFSuite: public ::testing::Test {
  public:
  protected:
    void SetUp() override { const auto *const inputOpenTest = INPUT_OPEN_TEST_PATH; }

    void TearDown() override {
        if (!deleteFile(pdfOutputPath)) {
            std::cerr << "Error occurred while trying to delete the PDF file." << '\n';
        }
    }
};

TEST_F(PDFSuite, SinglePDF) { createPDF(inputOpenTest, pdfOutputPath, tessdata_path); }