#include "../textract.h"
#include <algorithm>
#include <curl/curl.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <opencv2/core/check.hpp>
#include <tesseract/baseapi.h>

using namespace imgstr;

class MyTestSuite : public ::testing::Test {
public:
  const std::string tempDir = "tmp";
  const std::string path = "../../images/";
  const std::string inputFile = "../../images/imgtext.jpeg";
  const std::vector<std::string> images = {"screenshot.png", "imgtext.jpeg",
                                           "compleximgtext.png",
                                           "scatteredtext.png"};
  ImgProcessor imageTranslator = ImgProcessor();

protected:
  void SetUp() override { ::testing::internal::CaptureStderr(); }

  void TearDown() override {

    std::string captured_stdout_ = ::testing::internal::GetCapturedStderr();

    std::filesystem::remove_all(tempDir);
  }
};

TEST_F(MyTestSuite, EnvironmentTest) { EXPECT_NO_THROW(printSystemInfo()); }

TEST_F(MyTestSuite, ConvertImageToTextFile) {
  imageTranslator.convertImageToTextFile(inputFile, tempDir);
  bool fileExists = std::filesystem::exists(tempDir + "/" + "imgtext.txt");
  ASSERT_TRUE(fileExists);
}

TEST_F(MyTestSuite, WriteFileTest) {

  std::vector<std::string> paths;
  std::transform(images.begin(), images.end(), std::back_inserter(paths),
                 [&](const std::string &img) { return path + img; });

  EXPECT_NO_THROW(imageTranslator.addFiles(paths));
  EXPECT_NO_THROW(imageTranslator.convertImagesToTextFiles("tmp"));

  std::vector<int> test_lengths = {20, 1, 2, 9};

  for (size_t i = 0; i < images.size(); ++i) {
    std::size_t lastDot = images[i].find_last_of('.');
    std::string expectedOutputPath =
        tempDir + "/" + images[i].substr(0, lastDot) + ".txt";

    bool fileExists = std::filesystem::exists(expectedOutputPath);
    ASSERT_TRUE(fileExists);

    std::ifstream outputFile(expectedOutputPath);
    ASSERT_TRUE(outputFile.is_open());

    int lineCount = 0;
    std::string line;
    while (std::getline(outputFile, line))
      lineCount++;

    outputFile.close();
    EXPECT_GE(lineCount, test_lengths[i]);
  }
}

TEST_F(MyTestSuite, BasicAssertions) {
  tesseract::TessBaseAPI ocr;

  for (int i = static_cast<int>(ISOLang::en);
       i <= static_cast<int>(ISOLang::de); ++i) {
    ISOLang lang = static_cast<ISOLang>(i);
    const char *langStr = isoToTesseractLang(lang).c_str();

    EXPECT_NO_THROW((ocr.Init(nullptr, langStr)));
  }

  try {
    ocr.Init(nullptr, "nonexistent");
  } catch (const std::exception &e) {

    std::cout
        << "To enable all languages for Tesseract - make sure pack is "
           "installed.\nhttps://github.com/dataiku/dss-plugin-tesseract-ocr/"
           "tree/v1.0.2#specific-languages\n"
        << std::endl;

    EXPECT_STRNE(e.what(), "Please make sure the TESSDATA_PREFIX environment "
                           "variable is set to your \"tessdata\"directory.");
  }

  EXPECT_STRNE("hello", "world");
  EXPECT_EQ(7 * 6, 42);
}

std::string extractTextFromImageFileOpenCV(const std::string &file_path,
                                           const std::string &lang = "eng") {

  cv::Mat img = cv::imread(file_path);
  if (img.empty()) {
    throw std::runtime_error("Failed to load image: " + file_path);
  }
  cv::Mat gray;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
  cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

  tesseract::TessBaseAPI ocr;
  if (ocr.Init(nullptr, lang.c_str()) != 0) {
    throw std::runtime_error("Could not initialize tesseract.");
  }

  ocr.SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);

  std::string outText(ocr.GetUTF8Text());

  ocr.End();

  return outText;
}

std::string extractTextFromImageFileLeptonica(const std::string &file_path,
                                              const std::string &lang = "eng") {

  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
  if (api->Init(NULL, "eng")) {
    fprintf(stderr, "Could not initialize tesseract.\n");
    exit(1);
  }
  Pix *image = pixRead(file_path.c_str());

  api->SetImage(image);
  std::string outText(api->GetUTF8Text());
  outText = api->GetUTF8Text();

  api->End();
  delete api;
  pixDestroy(&image);
  return outText;
}

TEST_F(MyTestSuite, LEPTONICA_VS_OPENCV) {
  const std::string inputFile = "../../images/screenshot.png";

  auto start = getStartTime();
  auto res1 = extractTextFromImageFileLeptonica(inputFile);

  auto t1 = getDuration(start);
  std::cout << "Time Leptonica : " << t1 << '\n';

  auto s2 = getStartTime();
  auto res2 = extractTextFromImageFileOpenCV(inputFile);

  auto t2 = getDuration(start);
  std::cout << "Time OpenCV : " << t2 << '\n';
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
