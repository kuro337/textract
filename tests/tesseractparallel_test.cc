#include <conversion.h>
#include <generated_files.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-port.h>
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

// Fix issue with this fn handling Errors properly - shouldnt segfault

TEST_F(ParallelPerfTesseract, OEMvsLSTMAnalysisDebug) {
    ASSERT_NO_THROW(convertImagesToText(fileNames));
}

// TEST_F(ParallelPerfTesseract, OEMvsLSTMAnalysis) {
//     auto start = imgstr::getStartTime();

//     auto texts1 = convertImagesToText(fileNames);

//     auto time1  = imgstr::getDuration(start);
//     auto start2 = imgstr::getStartTime();

//     auto texts2 = convertImagesToTextTessPerfile(fileNames);

//     auto time2 = imgstr::getDuration(start);

//     std::cout << "Time Shared Instance : " << time1 << "ms\n";
//     std::cout << "Time Per Instance : " << time2 << "ms\n";

//     std::ranges::for_each(texts1, [](auto &x) {
//         std::cout << "Content:\n" << x << "\n\n";
//     });

//     std::ranges::for_each(texts2, [](auto &x) {
//         std::cout << "Content:\n" << x << "\n\n";
//     });
// }

auto main(int argc, char **argv) -> int {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
