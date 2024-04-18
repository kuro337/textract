

#include <filesystem>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <textract.h>

class ParallelPerfTesseract : public ::testing::Test {
  public:
    const std::string tempDir = "paralleloutput";
    // const std::string path = "../../../images/"; for ./build.sh tests
    const std::string path = "../../images/";   // cmbc build

    const std::vector<std::string> images = {"screenshot.png", "imgtext.jpeg",
                                             "compleximgtext.png",
                                             "scatteredtext.png"};

    imgstr::ImgProcessor imageTranslator;

    std::vector<std::string> qual_paths;

  protected:
    void SetUp() override {

        std::cout << "Current Path: " << std::filesystem::current_path()
                  << '\n';
        auto files = std::filesystem::directory_iterator(path);

        for (const auto &file : files) {
            std::cout << "File: " << file.path() << '\n';
        }

        for (const auto &image : images) {
            auto fpath = path + image;
            if (std::filesystem::exists(fpath)) {
                qual_paths.emplace_back(path + image);
            } else {
                std::cerr << "Invalid Path: " << fpath << '\n';
            }
        }
    }

    void TearDown() override {

        // std::string captured_stdout_ =
        // ::testing::internal::GetCapturedStderr();
        // std::filesystem::remove_all(tempDir);
    }
};

void
useTesseractInstancePerFile(const std::vector<std::string> &files,
                            const std::string &lang = "eng") {

    for (const auto &file : files) {
        auto start = imgstr::getStartTime();

        auto *api = new tesseract::TessBaseAPI();
        if (api->Init(nullptr, "eng") != 0) {
            fprintf(stderr, "Could not initialize tesseract.\n");
            exit(1);
        }
        imgstr::printDuration(start, "Tesserat Init Time Shared : ");
        Pix *image = pixRead(file.c_str());

        api->SetImage(image);

        char *rawText = api->GetUTF8Text();
        std::string outText(rawText);

        delete[] rawText;

        pixDestroy(&image);

        api->End();
        delete api;
    }
}

void
useTesseractSharedInstance(const std::vector<std::string> &files,
                           const std::string &lang = "eng") {

    auto start = imgstr::getStartTime();

    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(nullptr, "eng") != 0) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    imgstr::printDuration(start, "Tesserat Init Time Shared : ");

    for (const auto &file : files) {

        Pix *image = pixRead(file.c_str());

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

    auto time1 = imgstr::getDuration(start);
    std::cout << "Time Shared Instance : " << time1 << "ms\n";

    auto start2 = imgstr::getStartTime();

    useTesseractInstancePerFile(qual_paths);

    auto time2 = imgstr::getDuration(start);
    std::cout << "Time Per Instance : " << time2 << "ms\n";
}

auto
main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
