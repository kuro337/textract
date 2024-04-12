#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <omp.h>
#include <tesseract/baseapi.h>
#include <thread>
#include <vector>

class ThreadLocalTestSuite : public ::testing::Test {
  public:
    const std::string inputFile = "../../images/imgtext.jpeg";

    const std::string path = "../../images/";
    const std::vector<std::string> images = {"screenshot.png", "imgtext.jpeg",
                                             "compleximgtext.png",
                                             "scatteredtext.png"};

  protected:
    void SetUp() override { ::testing::internal::CaptureStderr(); }

    void TearDown() override {

        std::string captured_stdout_ = ::testing::internal::GetCapturedStderr();
    }
};

inline std::vector<unsigned char>
readBytesFromFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + filename);
    }
    return buffer;
}

std::string
getTextOCR(tesseract::TessBaseAPI *ocr,
           const std::vector<unsigned char> &file_content,
           const std::string &lang) {
    Pix *image =
        pixReadMem(reinterpret_cast<const l_uint8 *>(file_content.data()),
                   file_content.size());
    if (!image)
        throw std::runtime_error("Failed to load image from memory buffer");

    ocr->SetImage(image);
    std::string outText(ocr->GetUTF8Text());

    pixDestroy(&image);

    return outText;
};

struct TessBaseAPIWrapper {
    tesseract::TessBaseAPI *ocrPtr;

    TessBaseAPIWrapper(const std::string &lang = "eng")
        : ocrPtr(new tesseract::TessBaseAPI()) {
        if (ocrPtr->Init(nullptr, lang.c_str()) != 0) {
            delete ocrPtr;
            ocrPtr = nullptr;
            throw std::runtime_error("Could not initialize tesseract.");
        }
    }

    ~TessBaseAPIWrapper() {
        // std::cout << "DESTROYING OCR INSTANCE" << '\n';
        if (ocrPtr) {

            ocrPtr->Clear();
            ocrPtr->End();
            delete ocrPtr;
        }
    }

    tesseract::TessBaseAPI *operator->() const { return ocrPtr; }
};

tesseract::TessBaseAPI *
getThreadLocalOCR(const std::string &lang = "eng") {
    thread_local static TessBaseAPIWrapper ocrWrapper(lang);
    return ocrWrapper.ocrPtr;
}

void
processImage(const std::vector<unsigned char> &file_content,
             const std::string &lang) {
    try {
        auto ocr =
            getThreadLocalOCR(lang);   // Get the thread-local OCR instance

        std::string text = getTextOCR(ocr, file_content, lang);

    } catch (const std::runtime_error &e) {
    }
}

TEST_F(ThreadLocalTestSuite, ProcessMultipleImages) {
    std::vector<std::thread> threads;
    const int numThreads = 4;

    auto content = readBytesFromFile(inputFile);

    for (int i = 0; i < numThreads; ++i) {

        threads.emplace_back(processImage, content, "eng");
    }

    for (auto &thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    // using std::thread - the destructor is called fine
}

/* OPENMP PARALLELISM TESSERAT */

struct TessBaseAPIWrapperOpenMP {
    tesseract::TessBaseAPI *ocrPtr;

    TessBaseAPIWrapperOpenMP() : ocrPtr(nullptr) {}

    void init(const std::string &lang = "eng") {
        if (!ocrPtr) {
            ocrPtr = new tesseract::TessBaseAPI();
            if (ocrPtr->Init(nullptr, lang.c_str()) != 0) {
                delete ocrPtr;
                ocrPtr = nullptr;
                throw std::runtime_error("Could not initialize tesseract.");
            }
        }
    }

    ~TessBaseAPIWrapperOpenMP() {
        if (ocrPtr) {
            // std::cout << "Freed Tesserat Memory" << std::endl;
            ocrPtr->Clear();
            ocrPtr->End();
            delete ocrPtr;
        }
    }

    tesseract::TessBaseAPI *operator->() const { return ocrPtr; }
};

static TessBaseAPIWrapperOpenMP globalOcrWrapperOpenMP;
#pragma omp threadprivate(globalOcrWrapperOpenMP)

void
processImageOpenMP(const std::vector<unsigned char> &file_content,
                   const std::string &lang) {
    try {
        if (!globalOcrWrapperOpenMP.ocrPtr) {
            // std::cout   << "***\nnew instance tesserat\n***"<< std::endl;
            globalOcrWrapperOpenMP.init(lang);
        }
        std::string text =
            getTextOCR(globalOcrWrapperOpenMP.ocrPtr, file_content, lang);

    } catch (const std::runtime_error &e) {
        std::cout << "error generating text" << std::endl;
    }
}
TEST_F(ThreadLocalTestSuite, ProcessMultipleImagesOpenMP) {
    const int numThreads = 4;
    omp_set_num_threads(numThreads);

    auto content = readBytesFromFile(inputFile);

#pragma omp parallel
    {
#pragma omp for
        for (int i = 0; i < numThreads; ++i) {
            EXPECT_NO_THROW(processImageOpenMP(content, "eng"));
        }

        if (globalOcrWrapperOpenMP.ocrPtr) {
            globalOcrWrapperOpenMP.~TessBaseAPIWrapperOpenMP();
            globalOcrWrapperOpenMP.ocrPtr = nullptr;
        }
    }
}

void
cleanUpOpenMPTesserat() {
    const int numThreads = 4;
    omp_set_num_threads(numThreads);
#pragma omp parallel
    {
        if (globalOcrWrapperOpenMP.ocrPtr) {
            globalOcrWrapperOpenMP.~TessBaseAPIWrapperOpenMP();
            globalOcrWrapperOpenMP.ocrPtr = nullptr;
        }
    }
}

TEST_F(ThreadLocalTestSuite, MultipleFilesDivide) {
    const int numThreads = 4;
    omp_set_num_threads(numThreads);

#pragma omp parallel for
    for (int i = 0; i < images.size(); ++i) {

        std::string fullFilePath = path + images[i];

        auto content = readBytesFromFile(fullFilePath);

        EXPECT_NO_THROW(processImageOpenMP(content, "eng"));
    }

    EXPECT_NO_THROW(cleanUpOpenMPTesserat());
}

int
main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
