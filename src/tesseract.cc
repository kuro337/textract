
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