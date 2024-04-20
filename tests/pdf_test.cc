#include <gtest/gtest.h>
#include <leptonica/allheaders.h>
#include <llvm/Support/raw_ostream.h>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>

namespace {
    const auto *const inputOpenTest = INPUT_OPEN_TEST_PATH;
    const std::string pdfOutputPath = std::string(IMAGE_FOLDER_PATH) + "/my_first_tesseract_pdf";
    constexpr auto    tessdata_path = "/opt/homebrew/opt/tesseract/share/tessdata";
} // namespace
class PDFSuite: public ::testing::Test {
  public:
  protected:
    void SetUp() override { const auto *const inputOpenTest = INPUT_OPEN_TEST_PATH; }

    void TearDown() override {
        //    std::filesystem::remove_all(tempDir);
    }
};

void createPDF(const std::string &input_path, const std::string &output_path) {
    const char *datapath = "/opt/homebrew/opt/tesseract/share/tessdata";

    bool textonly = false;

    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(tessdata_path, "eng") != 0) {
        llvm::errs() << "Error: Could not initialize Tesseract OCR API." << '\n';
        llvm::errs() << "Tessdata path: " << tessdata_path << '\n';
        delete api; // Don't forget to delete api in case of failure
        return;
    }

    auto *renderer = new tesseract::TessPDFRenderer(output_path.c_str(), tessdata_path, textonly);

    llvm::outs() << "Input Path: " << input_path << '\n';
    bool succeed = api->ProcessPages(input_path.c_str(), nullptr, 0, renderer);
    if (!succeed) {
        llvm::errs() << "Error: Failed to process pages." << '\n';
        llvm::errs() << "Input file: " << input_path << '\n';
        llvm::errs() << "Output file: " << output_path << '\n';
    } else {
        llvm::outs() << "PDF creation succeeded." << '\n';
    }

    api->End();
    delete api;
    delete renderer;
}

TEST_F(PDFSuite, SinglePDF) { createPDF(inputOpenTest, pdfOutputPath); }