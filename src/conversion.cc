#include <array>
#include <conversion.h>
#include <iostream>
#include <leptonica/allheaders.h>
#include <llvm/Support/raw_ostream.h>
#include <ostream>
#include <string>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>
#include <unistd.h>

void cwd() {
    std::array<char, 1024> cwd {};
    if (getcwd(cwd.data(), cwd.size()) != nullptr) {
        std::cout << "Current working directory: " << cwd.data() << std::endl;
    } else {
        std::cerr << "getcwd() error" << std::endl;
    }
}
void createPDF(const std::string &input_path,
               const std::string &output_path,
               const char        *tessdata_path,
               bool               text_only) {
    // const char *datapath = "/opt/homebrew/opt/tesseract/share/tessdata";

    if (output_path.empty() || output_path == "." || output_path == "./" ||
        output_path.find_first_of(" ") != std::string::npos) {
        llvm::errs() << "Invalid Output Path passed, please pass a valid Absolute Path or File "
                        "Name :"
                     << output_path << '\n';

        return;
    }
    llvm::errs() << "Input : " << input_path << '\n';
    llvm::errs() << "Output : " << output_path << '\n';

    cwd();

    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(tessdata_path, "eng") != 0) {
        llvm::errs() << "Error: Could not initialize Tesseract OCR API." << '\n';
        llvm::errs() << "Tessdata path: " << tessdata_path << '\n';
        delete api; // Don't forget to delete api in case of failure
        return;
    }

    auto *renderer = new tesseract::TessPDFRenderer(output_path.c_str(), tessdata_path, text_only);

    bool succeed = api->ProcessPages(input_path.c_str(), nullptr, 0, renderer);
    if (!succeed) {
        llvm::errs() << "Error: Failed to process pages." << '\n';
    } else {
        llvm::outs() << "PDF creation succeeded." << '\n';
    }

    api->End();
    delete api;
    delete renderer;
}

auto extractTextFromImageFileLeptonica(const std::string &file_path, const std::string &lang)
    -> std::string {
    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(nullptr, "eng") != 0) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    Pix *image = pixRead(file_path.c_str());

    // fully automatic - suitable for single columns of text

    api->SetPageSegMode(tesseract::PSM_AUTO);
    api->SetImage(image);

    // Get the text from the image.
    char       *rawText = api->GetUTF8Text();
    std::string outText(rawText);
    delete[] rawText; // Free the memory allocated by GetUTF8Text

    api->End();
    delete api;
    pixDestroy(&image);
    return outText;
}

auto extractTextLSTM(const std::string &file_path, const std::string &lang) -> std::string {
    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) != 0) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    Pix *image = pixRead(file_path.c_str());

    api->SetImage(image);

    // Get the text from the image.
    char       *rawText = api->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText; // Free the memory allocated by GetUTF8Text

    api->End();
    delete api;
    pixDestroy(&image);
    return outText;
}

/// @brief Convert Image Files to Text and return the Strings
/// - Accepts Any collection with an Iterator and Types Convertable to a String View
/// - Compatible with : std::vector<string> or std::array<const char*> , ...
/// @tparam Container - vector<string> | array<const char*>
/// @param files
/// @param lang
/// @return std::vector<std::string>

/// @brief Convert Image Files to Text and return the Strings
/// - Accepts Any collection with an Iterator and Types Convertable to a String View
/// - Compatible with : std::vector<string> or std::array<const char*> , ...
/// @tparam Container - vector<string> | array<const char*>
/// @param files
/// @param lang
/// @return std::vector<std::string>
