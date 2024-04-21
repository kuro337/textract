
#include <leptonica/allheaders.h>
#include <llvm/Support/raw_ostream.h>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>

// tesseract.h
#ifndef TESSERACT_H
#define TESSERACT_H

void createPDF(const std::string &input_path,
               const std::string &output_path,
               const char        *tessdata_path,
               bool               text_only = false);

auto extractTextFromImageFileLeptonica(const std::string &file_path,
                                       const std::string &lang = "eng") -> std::string;

auto extractTextLSTM(const std::string &file_path, const std::string &lang = "eng") -> std::string;

#endif // TESSERACT_H