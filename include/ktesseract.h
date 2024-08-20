
// ktesseract.h
#ifndef KTESSERACT_H
#define KTESSERACT_H

#include "constants.h"
#include "util.h"
#include <allheaders.h>
#include <atomic>
#include <memory>
#include <omp.h>
#include <tesseract/baseapi.h>

#pragma region TESSERACT_OPENMP          /* Tesseract Implementation for Thread Local Tesseracts'  */

static std::atomic<int> TesseractThreadCount(0);

struct TesseractOCR {
    std::unique_ptr<tesseract::TessBaseAPI> ocrPtr;

    TesseractOCR(): ocrPtr(nullptr) {}

    TesseractOCR(const TesseractOCR &)                     = delete;
    auto operator=(const TesseractOCR &) -> TesseractOCR & = delete;
    TesseractOCR(TesseractOCR &&)                          = delete;
    auto operator=(TesseractOCR &&) -> TesseractOCR      & = delete;

    auto operator->() const -> tesseract::TessBaseAPI * { return ocrPtr.get(); }

    ~TesseractOCR() {
        serrfmt("{1} TesseractOCR Destructor Called on thread {0}{2}\n",
                omp_get_thread_num(),
                Ansi::WARNING,
                Ansi::END);

        if (ocrPtr != nullptr) {
            serrfmt("{0}Destructor call ocrPtr was not Null{1}\n", Ansi::WARNING, Ansi::END);
            ocrPtr->Clear();
            ocrPtr->End();
            TesseractThreadCount.fetch_sub(1, std::memory_order_relaxed);
            ocrPtr = nullptr;
        }
    }

    void init(const std::string &lang = "eng", ImgMode mode = ImgMode::document) {
        if (!ocrPtr) {
            serr << Ansi::WARNING << "Created New Tesseract" << Ansi::END << '\n';
            auto ptr = std::make_unique<tesseract::TessBaseAPI>();
            if (ptr->Init(nullptr, lang.c_str()) != 0) {
                throw std::runtime_error("Could not initialize tesseract.");
            }
            if (mode == ImgMode::image) {
                ptr->SetPageSegMode(tesseract::PSM_AUTO);
            }
            ocrPtr = std::move(ptr);
            TesseractThreadCount.fetch_add(1, std::memory_order_relaxed);
        }
    }
};

static std::unique_ptr<TesseractOCR> thread_local_tesserat = nullptr;

#pragma omp threadprivate(thread_local_tesserat)
inline void cleanupOpenMPTesserat() {
#pragma omp parallel
    {
        serrfmt("{1}cleanupOpenMPTesserat Called{2}\n{1}Cleanup: Thread Local OCR pointer "
                "clearing on thread {0}{2}",
                omp_get_thread_num(),
                Ansi::WARNING,
                Ansi::END);

        thread_local_tesserat.reset(); // <smartptr>.reset() invokes Destructor for Tesseract
    }
}

inline TesseractOCR *getThreadLocalTesserat() {
    if (thread_local_tesserat == nullptr) {
        thread_local_tesserat = std::make_unique<TesseractOCR>();
    }
    return thread_local_tesserat.get();
}

#pragma endregion

/* Leptonica reads 40% + faster than OpenCV */

/// @brief Creates a local threaded instance of a Tesseract and processes images converting them to
/// Text.
/// getThreadLocalTesserat() Retrives an instance of the Tesseract from local thread storage - to
/// achieve high throughput
/// @param file_content
/// @param lang
/// @param img_mode
/// @return std::string
inline std::string getTextOCR(const std::vector<unsigned char> &file_content,
                              const std::string                &lang,

                              ImgMode img_mode = ImgMode::document) {
#ifdef _DEBUGAPP

#endif
    auto *tesseract = getThreadLocalTesserat();

    if (tesseract->ocrPtr == nullptr) {
        tesseract->init(lang, img_mode);
    }

    Pix *image = pixReadMem(static_cast<const l_uint8 *>(file_content.data()), file_content.size());
    if (image == nullptr) {
        throw std::runtime_error("Failed to load image from memory "
                                 "buffer");
    }

    tesseract->ocrPtr->SetImage(image);

    char       *rawText = tesseract->ocrPtr->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;

    pixDestroy(&image);
    tesseract->ocrPtr->Clear();
    return outText;
};

std::string getTextOCRNoClear(const std::vector<unsigned char> &file_content,
                              const std::string                &lang = "eng",

                              ImgMode img_mode = ImgMode::document);

/// @brief Creates a local threaded instance of a Tesseract and processes images converting them to
/// Text. Does not clear the Tesseract - until all processing is done for the current Core.
/// getThreadLocalTesserat() Retrives an instance of the Tesseract from local thread storage - to
/// achieve high throughput
/// @param file_content
/// @param lang
/// @param img_mode
/// @return std::string
inline auto getTextOCRNoClear(const std::vector<unsigned char> &file_content,
                              const std::string                &lang,

                              ImgMode img_mode) -> std::string {
    auto *tesseract = getThreadLocalTesserat();

    if (tesseract->ocrPtr == nullptr) {
        tesseract->init(lang);
    }

    Pix *image = pixReadMem(static_cast<const l_uint8 *>(file_content.data()), file_content.size());
    if (image == nullptr) {
        throw std::runtime_error("Failed to load image from memory "
                                 "buffer");
    }

    tesseract->ocrPtr->SetImage(image);

    char *rawText = tesseract->ocrPtr->GetUTF8Text();

    std::string outText(rawText);

    delete[] rawText;
    pixDestroy(&image);

    return outText;
};

/// @brief Read Data from a File & Get Text from a Single Image File using the Thread Local
/// Tesseract
/// @param file_path
/// @param lang
/// @return std::string
inline auto getTextImgFile(const std::string &file_path, const std::string &lang) -> std::string {
    auto *tesseract = getThreadLocalTesserat();

    if (tesseract->ocrPtr == nullptr) {
        tesseract->init(lang);
    }

    Pix *image = pixRead(file_path.c_str());

    tesseract->ocrPtr->SetImage(image);

    char       *rawText = tesseract->ocrPtr->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;

    pixDestroy(&image);

    tesseract->ocrPtr->Clear();

    return outText;
}

#endif // KTESSERACT_H
