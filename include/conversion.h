
#include <leptonica/allheaders.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>
#include <vector>

// conversion.h
#ifndef CONVERSION_H
#define CONVERSION_H

/// @brief Generate a PDF from an Image or TexT File and Output to Disk
/// @param input_path
/// @param output_path
/// @param tessdata_path
/// @param text_only
void createPDF(const std::string &input_path,
               const std::string &output_path,
               const char        *tessdata_path,
               bool               text_only = false);

/// @brief Perform Text Extraction using Leptonica
/// @param file_path
/// @param lang
/// @return std::string
auto extractTextFromImageFileLeptonica(const std::string &file_path,
                                       const std::string &lang = "eng") -> std::string;

/// @brief Perform Text Extraction using the LSTM Algo suited for Fuzzy Matches
/// @param file_path
/// @param lang
/// @return std::string
auto extractTextLSTM(const std::string &file_path, const std::string &lang = "eng") -> std::string;

/// @brief Convert Image Files to Text and return the Strings
/// - Accepts Any collection with an Iterator and Types Convertable to a String View
/// - Compatible with : std::vector<string> or std::array<const char*> , ...
/// @tparam Container - vector<string> | array<const char*>
/// @param files
/// @param lang
/// @return std::vector<std::string>
template <typename Container>
auto convertImagesToText(const Container   &files,
                         const std::string &lang = "eng") -> std::vector<std::string> {
    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(nullptr, "eng") != 0) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    std::vector<std::string> imageText;
    // works with std::vector<string> or std::array<const char*> etc.
    for (const auto &file: files) {
        const char *filename = nullptr; // Variable to hold the pointer to the filename
        if constexpr (std::is_same_v<std::decay_t<decltype(file)>, std::string>) {
            filename = file.c_str();
        } else if constexpr (std::is_same_v<std::decay_t<decltype(file)>, const char *>) {
            filename = file;
        } else {
            static_assert(std::is_same_v<decltype(file), void>,
                          "Unsupported type : Type must be std::string or const char*.");
        }

        Pix *image = pixRead(filename);
        api->SetImage(image);
        pixDestroy(&image);
        char       *rawText = api->GetUTF8Text();
        std::string outText(rawText);
        imageText.emplace_back(std::move(outText));
        delete[] rawText;
        pixDestroy(&image);
    }
    api->End();
    delete api;
    return imageText;
};
/// @brief Convert Image Files to Text and return the Strings
/// - Accepts Any collection with an Iterator and Types Convertable to a String View
/// - Compatible with : std::vector<string> or std::array<const char*> , ...
/// @tparam Container - vector<string> | array<const char*>
/// @param files
/// @param lang
/// @return std::vector<std::string>
template <typename Container>
auto convertImagesToTextTessPerfile(const Container   &files,
                                    const std::string &lang = "eng") -> std::vector<std::string> {
    std::vector<std::string> imageText;
    for (const auto &file: files) {
        // Determine if we need to call .c_str() or it's directly usable
        const char *filename = nullptr; // Variable to hold the pointer to the filename
        if constexpr (std::is_same_v<std::decay_t<decltype(file)>, std::string>) {
            filename = file.c_str();
        } else if constexpr (std::is_same_v<std::decay_t<decltype(file)>, const char *>) {
            filename = file;
        } else {
            static_assert(std::is_same_v<decltype(file), void>,
                          "Unsupported type : Type must be std::string or const char*.");
        }

        auto *api = new tesseract::TessBaseAPI();
        if (api->Init(nullptr, "eng") != 0) {
            fprintf(stderr, "Could not initialize tesseract.\n");
            exit(1);
        }
        Pix *image = pixRead(filename);
        api->SetImage(image);
        char       *rawText = api->GetUTF8Text();
        std::string outText(rawText);
        imageText.emplace_back(std::move(outText));
        delete[] rawText;
        pixDestroy(&image);
        api->End();
        delete api;
    }
    return imageText;
}

#endif // CONVERSION_H