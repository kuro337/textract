#include <algorithm>
#include <cstdint>
#include <curl/curl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <src/fs.h>
#include <tesseract/baseapi.h>
#include <textract.h>

#ifdef _USE_OPENCV
#include <opencv2/core/check.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

class MyTestSuite: public ::testing::Test {
  public:
    MyTestSuite() {
        const std::string imgFolder  = "../../images";
        auto              imagePaths = getFilePaths(imgFolder);

        if (imagePaths) {
            fpaths = imagePaths.get();
        }

        for (const auto &e: fpaths) {
            std::cout << e << '\n';
        }
    }

    std::vector<std::string> fpaths;

    const std::string inputOpenTest = "../../images/screenshot.png";

    std::string tempDir = "tmpOCR";

  protected:
    void TearDown() override {
    }
};

TEST_F(MyTestSuite, EnvironmentTest) {
    std::cout << "Printing Sys Info" << std::endl;
    EXPECT_NO_THROW(imgstr::printSystemInfo());

    std::cout << "Printed Sys Info" << std::endl;
}

TEST_F(MyTestSuite, ConvertImageToTextFile) {
    std::cout << "Running Convert Test" << std::endl;

    imgstr::ImgProcessor imageTranslator;

    imageTranslator.convertImageToTextFile(fpaths[0], tempDir);
    std::filesystem::path p(fpaths[0]);

    bool fileExists = std::filesystem::exists(tempDir + "/" + p.filename().string());
    std::cout << "Tempdir File check: " << tempDir + "/" + p.filename().string() << '\n';

    ASSERT_TRUE(fileExists);
}

// TEST_F(MyTestSuite, WriteFileTest) {
//     // std::vector<std::string> paths;
//     // std::transform(images.begin(), images.end(),
//     std::back_inserter(paths), [&](const std::string &img) {
//     //     return path + img;
//     // });
//     imgstr::ImgProcessor imageTranslator;
//     EXPECT_NO_THROW(imageTranslator.addFiles(fpaths));
//     EXPECT_NO_THROW(imageTranslator.convertImagesToTextFiles(tempDir));

//     std::vector<int> test_lengths = {20, 1, 1, 9};

//         for (size_t i = 0; i < fpaths.size(); ++i) {
//             std::size_t lastDot = fpaths[i].find_last_of('.');
//             //            std::string expectedOutputPath = tempDir + "/" +
//             fpaths[i].substr(0, lastDot) + ".txt";

//             std::filesystem::path p(fpaths[i]);
//             auto                  expectedOutputPath = tempDir + "/" +
//             p.filename().string(); std::filesystem::path
//             p2(expectedOutputPath); p2.replace_extension(".txt");
//             expectedOutputPath = p2.string();
//             bool fileExists    = std::filesystem::exists(expectedOutputPath);
//             //    imageTranslator.convertImageToTextFile(fpaths[0], tempDir +
//             "/" + p.filename().string());

//             std::cout << "File Exists WriteFile:" << expectedOutputPath <<
//             '\n'; ASSERT_TRUE(fileExists);

//             std::ifstream outputFile(expectedOutputPath);
//             ASSERT_TRUE(outputFile.is_open());

//             int         lineCount = 0;
//             std::string line;
//                 while (std::getline(outputFile, line)) {
//                     lineCount++;
//                 }

//             outputFile.close();
//             EXPECT_GE(lineCount, test_lengths[i]);
//         }
// }

TEST_F(MyTestSuite, BasicAssertions) {
    tesseract::TessBaseAPI ocr;

    EXPECT_NO_THROW((ocr.Init(nullptr, imgstr::ISOLanguage::eng)));
    EXPECT_NO_THROW((ocr.Init(nullptr, imgstr::ISOLanguage::hin)));
    EXPECT_NO_THROW((ocr.Init(nullptr, imgstr::ISOLanguage::chi)));
    EXPECT_NO_THROW((ocr.Init(nullptr, imgstr::ISOLanguage::ger)));
    EXPECT_NO_THROW((ocr.Init(nullptr, imgstr::ISOLanguage::fra)));
    EXPECT_NO_THROW((ocr.Init(nullptr, imgstr::ISOLanguage::esp)));

    try {
        ocr.Init(nullptr, "nonexistent");
    } catch (const std::exception &e) {
        std::cout << "To enable all languages for Tesseract - make sure pack is "
                     "installed.\nhttps://github.com/dataiku/"
                     "dss-plugin-tesseract-ocr/"
                     "tree/v1.0.2#specific-languages\n"
                  << std::endl;

        EXPECT_STRNE(e.what(),
                     "Please make sure the TESSDATA_PREFIX environment "
                     "variable is set to your \"tessdata\"directory.");
    }

    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}

// auto extractTextFromImageFileLeptonica(const std::string &file_path, const
// std::string &lang = "eng") -> std::string {
//     auto *api = new tesseract::TessBaseAPI();
//         if (api->Init(nullptr, "eng") != 0) {
//             fprintf(stderr, "Could not initialize tesseract.\n");
//             exit(1);
//     }
//     Pix *image = pixRead(file_path.c_str());

//     // fully automatic - suitable for single columns of text

//     api->SetPageSegMode(tesseract::PSM_AUTO);

//     api->SetImage(image);
//     std::string outText(api->GetUTF8Text());
//     outText = api->GetUTF8Text();

//     api->End();
//     delete api;
//     pixDestroy(&image);
//     return outText;
// }

// auto extractTextLSTM(const std::string &file_path, const std::string &lang =
// "eng") -> std::string {
//     auto *api = new tesseract::TessBaseAPI();
//         if (api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) != 0) {
//             fprintf(stderr, "Could not initialize tesseract.\n");
//             exit(1);
//     }
//     Pix *image = pixRead(file_path.c_str());

//     api->SetImage(image);
//     std::string outText(api->GetUTF8Text());
//     outText = api->GetUTF8Text();

//     api->End();
//     delete api;
//     pixDestroy(&image);
//     return outText;
// }

// TEST_F(MyTestSuite, OEMvsLSTMAnalysis) {
//     auto start = imgstr::getStartTime();
//     auto res1  = extractTextFromImageFileLeptonica(fpaths[1]);

//     std::cout << res1 << '\n';

//     auto time1 = imgstr::getDuration(start);
//     std::cout << "Time Leptonica : " << time1 << '\n';

//     auto start2 = imgstr::getStartTime();
//     auto res2   = extractTextLSTM(fpaths[1]);

//     std::cout << res2 << '\n';

//     auto time2 = imgstr::getDuration(start);
//     std::cout << "Time LSTM: " << time2 << '\n';
// }

#ifdef _USE_OPENCV

std::string extractTextFromImageFileOpenCV(const std::string &file_path, const std::string &lang = "eng") {
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
#endif

#ifdef _USE_OPENCV

TEST_F(MyTestSuite, LEPTONICA_VS_OPENCV) {
    auto start = getStartTime();
    auto res1  = extractTextFromImageFileLeptonica(inputOpenTest);

    auto t1 = getDuration(start);
    std::cout << "Time Leptonica : " << t1 << '\n';

    auto s2   = getStartTime();
    auto res2 = extractTextFromImageFileOpenCV(inputOpenTest);

    auto t2 = getDuration(start);
    std::cout << "Time OpenCV : " << t2 << '\n';
}

#endif

auto main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
