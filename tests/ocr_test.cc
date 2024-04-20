#include <curl/curl.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <llvm/Support/FileSystem.h>
#include <src/fs.h>
#include <tesseract/baseapi.h>
#include <textract.h>

#ifdef _USE_OPENCV
#include <opencv2/core/check.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace {
    constexpr auto *const imgFolder     = IMAGE_FOLDER_PATH;
    constexpr auto *const inputOpenTest = INPUT_OPEN_TEST_PATH;
} // namespace
class ImageProcessingTests: public ::testing::Test {
  public:
    ImageProcessingTests() { fpaths = getFilePaths(imgFolder).get(); }

    std::vector<std::string> fpaths;

    std::string tempDir = "tmpOCR";

  protected:
    void TearDown() override {
        if (auto err = deleteDirectory(tempDir); err) {
            FAIL() << "Cleanup Failure" << llvm::toString(std::move(err)) << "\n";
        }
    }
};

TEST_F(ImageProcessingTests, EnvironmentTest) {
    std::cout << "Printing Sys Info" << std::endl;
    EXPECT_NO_THROW(imgstr::printSystemInfo());

    std::cout << "Printed Sys Info" << std::endl;
}

TEST_F(ImageProcessingTests, ConvertSingleImageToTextFile) {
    imgstr::ImgProcessor imageTranslator;

    imageTranslator.convertImageToTextFile(fpaths[0], tempDir);

    auto filename = llvm::SmallString<256>(llvm::sys::path::filename(fpaths[0]));

    llvm::sys::path::replace_extension(filename, ".txt");

    auto fname = tempDir + "/" + filename;
    llvm::outs() << "Single Image Test File: " << fpaths[0] << ", LF: " << fname << '\n';

    bool fileExists = llvm::sys::fs::exists(tempDir + "/" + filename);

    ASSERT_TRUE(fileExists);
}

TEST_F(ImageProcessingTests, WriteFileTest) {
    // std::vector<std::string> paths;
    // std::transform(images.begin(), images.end(),

    //    std::back_inserter(paths), [&](const std::string &img) {
    //     return path + img;
    // });

    imgstr::ImgProcessor imageTranslator;

    EXPECT_NO_THROW(imageTranslator.addFiles(fpaths));
    EXPECT_NO_THROW(imageTranslator.convertImagesToTextFiles(tempDir));

    for (const auto &fpath: fpaths) {
        std::filesystem::path p(fpath);
        auto                  expectedOutputPath = tempDir + "/" + p.filename().string();

        std::filesystem::path p2(expectedOutputPath);

        p2.replace_extension(".txt");

        expectedOutputPath = p2.string();

        bool fileExists = std::filesystem::exists(expectedOutputPath);

        if (!fileExists) {
            llvm::outs() << "FILE DOES NOT EXIST: " << expectedOutputPath << '\n';
        }

        auto outputPath = tempDir + "/" + p.filename().string();

        llvm::outs() << "Expected Output Path: " << expectedOutputPath << '\n';
        llvm::outs() << "Computed Output Path: " << outputPath << '\n';

        ASSERT_NO_THROW(imageTranslator.convertImageToTextFile(fpath, outputPath));

        std::ifstream outputFile(expectedOutputPath);
        ASSERT_NO_THROW(outputFile.is_open());

        int         lineCount = 0;
        std::string line;
        while (std::getline(outputFile, line)) {
            lineCount++;
        }

        outputFile.close();
    }
}

TEST_F(ImageProcessingTests, CheckForUniqueTextFile) {
    llvm::SmallString<128> dupePath(tempDir);
    llvm::sys::path::append(dupePath, "dupescreenshot.txt");

    llvm::SmallString<128> screenPath(tempDir);
    llvm::sys::path::append(screenPath, "screenshot.txt");

    bool dupeExists   = llvm::sys::fs::exists(dupePath);
    bool screenExists = llvm::sys::fs::exists(screenPath);

    EXPECT_TRUE(dupeExists != screenExists)
        << "Only one should exist as we cache by the Image Hash. Concurrency can cause both to "
           "Exist - so not an issue if both Exist.";
}

TEST_F(ImageProcessingTests, BasicAssertions) {
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

std::string
    extractTextFromImageFileOpenCV(const std::string &file_path, const std::string &lang = "eng") {
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
