

#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <textract.h>

class ParallelPerfTesseract : public ::testing::Test {
public:
  const std::string tempDir = "paralleloutput";
  const std::string path = "../../../images/";
  const std::vector<std::string> images = {"screenshot.png", "imgtext.jpeg",
                                           "compleximgtext.png",
                                           "scatteredtext.png"};

  imgstr::ImgProcessor imageTranslator = imgstr::ImgProcessor();

  std::vector<std::string> qual_paths;

protected:
  void SetUp() override {

    for (auto it = images.begin(); it != images.end(); it++) {
      qual_paths.emplace_back(path + (*it));
    }
  }

  void TearDown() override {

    // std::string captured_stdout_ = ::testing::internal::GetCapturedStderr();
    // std::filesystem::remove_all(tempDir);
  }
};

void useTesseractInstancePerFile(const std::vector<std::string> &files,
                                 const std::string &lang = "eng") {

  for (auto it = files.begin(); it != files.end(); it++) {
    auto start = imgstr::getStartTime();

    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    if (api->Init(NULL, "eng")) {
      fprintf(stderr, "Could not initialize tesseract.\n");
      exit(1);
    }
    imgstr::printDuration(start, "Tesserat Init Time Shared : ");
    Pix *image = pixRead((*it).c_str());

    api->SetImage(image);

    char *rawText = api->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;

    pixDestroy(&image);

    api->End();
    delete api;
  }
}

void useTesseractSharedInstance(const std::vector<std::string> &files,
                                const std::string &lang = "eng") {

  auto start = imgstr::getStartTime();

  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
  if (api->Init(NULL, "eng")) {
    fprintf(stderr, "Could not initialize tesseract.\n");
    exit(1);
  }
  imgstr::printDuration(start, "Tesserat Init Time Shared : ");

  for (auto it = files.begin(); it != files.end(); it++) {

    Pix *image = pixRead((*it).c_str());

    api->SetImage(image);

    pixDestroy(&image);

    char *rawText = api->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;

    pixDestroy(&image);
  }

  api->End();
  delete api;
};

TEST_F(ParallelPerfTesseract, OEMvsLSTMAnalysis) {

  auto start = imgstr::getStartTime();

  useTesseractSharedInstance(qual_paths);

  auto t1 = imgstr::getDuration(start);
  std::cout << "Time Shared Instance : " << t1 << "ms\n";

  auto s2 = imgstr::getStartTime();

  useTesseractInstancePerFile(qual_paths);

  auto t2 = imgstr::getDuration(start);
  std::cout << "Time Per Instance : " << t2 << "ms\n";
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
