
#include <leptonica/allheaders.h>
#include <llvm/Support/raw_ostream.h>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>

void createPDF(const std::string &input_path,
               const std::string &output_path,
               const char        *tessdata_path,
               bool               text_only = false) {
    // const char *datapath = "/opt/homebrew/opt/tesseract/share/tessdata";

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
        llvm::errs() << "Input file: " << input_path << '\n';
        llvm::errs() << "Output file: " << output_path << '\n';
    } else {
        llvm::outs() << "PDF creation succeeded." << '\n';
    }

    api->End();
    delete api;
    delete renderer;
}

auto extractTextFromImageFileLeptonica(const std::string &file_path,
                                       const std::string &lang = "eng") -> std::string {
    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(nullptr, "eng") != 0) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    Pix *image = pixRead(file_path.c_str());

    // fully automatic - suitable for single columns of text

    api->SetPageSegMode(tesseract::PSM_AUTO);

    api->SetImage(image);
    std::string outText(api->GetUTF8Text());
    outText = api->GetUTF8Text();

    api->End();
    delete api;
    pixDestroy(&image);
    return outText;
}

auto extractTextLSTM(const std::string &file_path, const std::string &lang = "eng") -> std::string {
    auto *api = new tesseract::TessBaseAPI();
    if (api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY) != 0) {
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
