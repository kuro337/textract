#include <gtest/gtest.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>

class PDFSuite : public ::testing::Test {
public:
  const std::string inputOpenTest = "../../../images/screenshot.png";
  const std::string pdfOutputPath =
      "../../../images/processed/my_first_tesseract_pdf";

  const std::string tessdata_path =
      "/opt/homebrew/opt/tesseract/share/tessdata";

protected:
  void SetUp() override {}

  void TearDown() override {

    //    std::filesystem::remove_all(tempDir);
  }
};

void createPDF(const std::string &input_path, const std::string &output_path) {
  const char *datapath = "/opt/homebrew/opt/tesseract/share/tessdata";

  bool textonly = false;

  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
  if (api->Init(datapath, "eng")) {
    fprintf(stderr, "Could not initialize tesseract.\n");
    delete api; // Don't forget to delete api in case of failure
    return;
  }

  tesseract::TessPDFRenderer *renderer =
      new tesseract::TessPDFRenderer(output_path.c_str(), datapath, textonly);

  bool succeed = api->ProcessPages(input_path.c_str(), nullptr, 0, renderer);
  if (!succeed) {
    fprintf(stderr, "Error during processing.\n");
  }

  api->End();
  delete api;
  delete renderer;
}

TEST_F(PDFSuite, SinglePDF) { createPDF(inputOpenTest, pdfOutputPath); }