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
} // namespace
class PDFSuite: public ::testing::Test {
  public:
  protected:
    void SetUp() override { const auto *const inputOpenTest = INPUT_OPEN_TEST_PATH; }

    void TearDown() override {
        // Attempt to delete the file and capture the result as an error.
        llvm::Error deletionResult = deleteFile(pdfOutputPath + ".pdf");

        if (deletionResult) {
            llvm::handleAllErrors(std::move(deletionResult), [&](const llvm::StringError &err) {
                llvm::errs() << "Error occurred while trying to delete the PDF file: "
                             << err.getMessage() << '\n';
            });
        }
    }
};

TEST_F(PDFSuite, SinglePDF) { createPDF(inputOpenTest, pdfOutputPath, TESSDATA_PREFIX); }