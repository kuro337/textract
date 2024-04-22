#include <conversion.h>
#include <generated_files.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
#include <iostream>
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

// // Works with std::vector<string> or std::array<const char*> etc.
// // Only Requirement : Container Elems can convert to std::string_view
// template <typename Container>
// auto useTesseractInstancePerFile(const Container   &files,
//                                  const std::string &lang = "eng") -> std::vector<std::string> {
//     std::vector<std::string> imageText;
//     for (const auto &file: files) {
//         auto start = imgstr::getStartTime();

//         // Determine if we need to call .c_str() or it's directly usable
//         const char *filename = nullptr; // Variable to hold the pointer to the filename
//         if constexpr (std::is_same_v<std::decay_t<decltype(file)>, std::string>) {
//             filename = file.c_str();
//         } else if constexpr (std::is_same_v<std::decay_t<decltype(file)>, const char *>) {
//             filename = file;
//         }

//         auto *api = new tesseract::TessBaseAPI();
//         if (api->Init(nullptr, "eng") != 0) {
//             fprintf(stderr, "Could not initialize tesseract.\n");
//             exit(1);
//         }
//         imgstr::printDuration(start, "Tesserat Init Time Shared : ");
//         Pix *image = pixRead(filename);

//         api->SetImage(image);

//         char       *rawText = api->GetUTF8Text();
//         std::string outText(rawText);

//         imageText.emplace_back(std::move(outText));

//         delete[] rawText;

//         pixDestroy(&image);

//         api->End();
//         delete api;
//     }

//     return imageText;
// }

// // Works with std::vector<string> or std::array<const char*> etc.
// // Only Requirement : Container Elems can convert to std::string_view
// template <typename Container>
// auto useTesseractSharedInstance(const Container   &files,
//                                 const std::string &lang = "eng") -> std::vector<std::string> {
//     auto  start = imgstr::getStartTime();
//     auto *api   = new tesseract::TessBaseAPI();
//     if (api->Init(nullptr, "eng") != 0) {
//         fprintf(stderr, "Could not initialize tesseract.\n");
//         exit(1);
//     }
//     imgstr::printDuration(start, "Tesserat Init Time Shared : ");

//     std::vector<std::string> imageText;

//     // works with std::vector<string> or std::array<const char*> etc.
//     for (const auto &file: files) {
//         const char *filename = nullptr; // Variable to hold the pointer to the filename

//         if constexpr (std::is_same_v<std::decay_t<decltype(file)>, std::string>) {
//             filename = file.c_str();
//         } else if constexpr (std::is_same_v<std::decay_t<decltype(file)>, const char *>) {
//             filename = file;
//         }

//         Pix *image = pixRead(filename);

//         api->SetImage(image);

//         pixDestroy(&image);
//         char       *rawText = api->GetUTF8Text();
//         std::string outText(rawText);

//         imageText.emplace_back(std::move(outText));

//         delete[] rawText;

//         pixDestroy(&image);
//     }

//     api->End();
//     delete api;

//     return imageText;
// };

TEST_F(ParallelPerfTesseract, OEMvsLSTMAnalysis) {
    auto start = imgstr::getStartTime();

    auto texts1 = convertImagesToText(fileNames);

    auto time1  = imgstr::getDuration(start);
    auto start2 = imgstr::getStartTime();

    auto texts2 = convertImagesToTextTessPerfile(fileNames);

    auto time2 = imgstr::getDuration(start);

    std::cout << "Time Shared Instance : " << time1 << "ms\n";
    std::cout << "Time Per Instance : " << time2 << "ms\n";

    std::ranges::for_each(texts1, [](auto &x) {
        std::cout << "Content:\n" << x << "\n\n";
    });

    std::ranges::for_each(texts2, [](auto &x) {
        std::cout << "Content:\n" << x << "\n\n";
    });
}

auto main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
