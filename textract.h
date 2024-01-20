
#ifndef TEXTRACT_H
#define TEXTRACT_H

#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <cstddef>
#include <folly/AtomicUnorderedMap.h>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <omp.h>
#include <optional>
#include <string>
#include <tesseract/baseapi.h>
#include <vector>

namespace imgstr {

#pragma region TEXT_SIMILARITY  /* Text Similarity Declarations */

size_t levenshteinScore(std::string a, std::string b);

#pragma endregion

#pragma region CRYPTOGRAPHY     /* Cryptography Declarations */

std::string computeSHA256(const std::vector<uchar> &data);

std::string computeSHA256(const std::string &filePath);

#pragma endregion

#pragma region SYSTEM_UTILS     /* System Environment helpers */

void printSystemInfo();

const std::string delimiter =
    "\x1b[90m***********************************************************";
const std::string check_mark = "\x1b[32m✔\x1b[0m";
const std::string green = "\x1b[92m";
const std::string end = "\x1b[0m";
const std::string yellow = "\x1b[93m";

#pragma endregion

#pragma region FILE_IO         /* File IO helpers */

std::vector<uchar> readBytesFromFile(const std::string &filename);

void writeToNewFile(const std::string &content, const std::string &output_path);

#pragma endregion

#pragma region OPENCV_UTILS    /* OpenCV Declarations */

enum class ISOLang { en, es, fr, hi, zh, de };

inline std::string isoToTesseractLang(ISOLang isoLang) {
  switch (isoLang) {
  case ISOLang::en:
    return "eng";
  case ISOLang::es:
    return "spa";
  case ISOLang::fr:
    return "fra";
  case ISOLang::de:
    return "deu";
  case ISOLang::zh:
    return "chi_sim";
  case ISOLang::hi:
    return "hin";
  default:
    return "eng";
  }
}

std::string extractTextFromImageBytes(const std::vector<uchar> &file_content,
                                      const std::string &lang);

std::string extractTextFromImageFile(const std::string &file_path,
                                     const std::string &lang);

std::string extractTextFromImageFile(const std::string &file_path,
                                     ISOLang lang);

#pragma endregion

/*

ImgProcessor : Core header class

Provides an efficient, high performance implementation of Text
Extraction from Images.

Supports Parallelized Image Processing and maintains an in-memory cache.

Uses an Atomic Unordered Map for Safe Wait-Free parallel access to ensure images
are not processed twice

Cache retrieval logic is determined by the SHA256 hash of the Image bytes

The SHA256 Byte Hash enables duplicate images to not be processed even if the
file names or paths differ.

*/

#pragma region imgstr_core      /* Core Class for Image Processing and Text Extraction */

struct Image {
  std::string path;
  std::string name;
  std::size_t byte_size;
  std::string text_content;
  std::string content_fuzzhash;
  std::string image_sha256;
};

class ImgProcessor {

private:
  std::string dir;

  std::vector<std::string> files;

  folly::AtomicUnorderedInsertMap<std::string, std::string> cache;

  std::optional<std::string> getFromCacheIfExists(const std::string &img_sha) {
    auto text_from_cache = cache.find(img_sha);

    if (text_from_cache != cache.end()) {
      return text_from_cache->second;
    }
    return std::nullopt;
  }

  std::vector<std::string> processCurrentFiles() {
    std::vector<std::string> processedText;
    if (files.empty()) {
      std::cout << "Files are empty" << std::endl;
      return {};
    }
    for (const auto &file : files) {

      try {
        auto data = readBytesFromFile(file);
        std::string img_hash = computeSHA256(data);
        auto text_from_cache = getFromCacheIfExists(img_hash);

        if (text_from_cache) {
          printCacheHit(file);
          processedText.emplace_back(*text_from_cache);
        } else {
          std::string img_text = extractTextFromImageBytes(data, "eng");
          cache.emplace(img_hash, img_text);
          processedText.emplace_back(img_text);
        }

      } catch (const std::exception &e) {
        std::cout << "Failed to Extract Text from Image file: " << file
                  << ". Error: " << e.what() << '\n';
      }
    }

    return processedText;
  }

  void printCacheHit(const std::string &file) {
    std::cout << delimiter << '\n'
              << check_mark << green << "  Image Already Processed : " << end
              << file << '\n';
  }

public:
  ImgProcessor(size_t capacity = 1000)
      : files(),
        cache(folly::AtomicUnorderedInsertMap<std::string, std::string>(
            capacity)) {}

  void setDir(const std::string dir_path) { dir = dir_path; }

  void addFile(const std::string &file_path) { files.push_back(file_path); }

  void resetCache(size_t new_capacity) {
    cache =
        folly::AtomicUnorderedInsertMap<std::string, std::string>(new_capacity);
  }

  template <typename... FileNames>
  std::vector<std::string> processImages(FileNames... fileNames) {
    addFiles({fileNames...}); // Use initializer_list to unpack the variadic

    return processCurrentFiles();
  }

  void addFiles(std::initializer_list<std::string> fileList) {
    for (const auto &file : fileList) {
      this->files.push_back(file);
    }
  }

  void addFiles(const std::vector<std::string> &fileList) {
    for (const auto &file : fileList) {
      files.push_back(file);
    }
  }

  void printFiles() {
    for (const auto &file : files) {
      std::cout << file << std::endl;
    }
  }

  // pass file path , get Text
  std::string getImageText(const std::string &file_path,
                           ISOLang lang = ISOLang::en) {
    try {

      // if already processed - return else process image and add to cache
      auto data = readBytesFromFile(file_path);
      std::string img_hash = computeSHA256(data);
      auto text_from_cache = getFromCacheIfExists(img_hash);

      if (text_from_cache) {
        printCacheHit(file_path);
        return *text_from_cache;
      } else {
        std::string img_text =
            extractTextFromImageBytes(data, isoToTesseractLang(lang));
        cache.emplace(img_hash, img_text);

        return img_text;
      }
    } catch (const std::exception &e) {
      std::cout << "Failed to Extract Text from Image file: " << file_path
                << ". Error: " << e.what() << '\n';
    }

    return nullptr;
  }

  void writeTextToFile(const std::string &content,
                       const std::string &output_path) {

    if (std::filesystem::exists(output_path)) {
      std::cerr << "Error: File already exists - " << output_path << std::endl;
      return;
    }

    std::ofstream outFile(output_path);
    if (!outFile) {
      std::cerr << "Error opening file: " << output_path << std::endl;
      return;
    }
    outFile << content;
  }

  void convertImageToTextFile(const std::string &input_file,
                              const std::string &output_dir = "",
                              ISOLang lang = ISOLang::en) {
#ifdef _WIN32
    std::string path_separator = "\\";
#else
    std::string path_separator = "/";
#endif

    // create output dir if not exists
    if (!output_dir.empty() && !std::filesystem::exists(output_dir)) {
      std::filesystem::create_directories(output_dir);
    }
    std::string outputFilePath = output_dir;
    if (!output_dir.empty() && output_dir.back() != path_separator.back()) {
      outputFilePath += path_separator;
    }

    std::size_t lastSlash = input_file.find_last_of("/\\");
    std::size_t lastDot = input_file.find_last_of('.');
    std::string filename =
        input_file.substr(lastSlash + 1, lastDot - lastSlash - 1);
    outputFilePath += filename + ".txt";

    std::string converted = getImageText(input_file, lang);
    writeTextToFile(converted, outputFilePath);
  }

  void convertImagesToTextFiles(const std::string &output_dir = "",
                                ISOLang lang = ISOLang::en) {
#ifdef _WIN32
    std::string path_separator = "\\";
#else
    std::string path_separator = "/";
#endif

    // create output dir if not exists
    if (!output_dir.empty() && !std::filesystem::exists(output_dir)) {
      std::filesystem::create_directories(output_dir);
    }

#pragma omp parallel for
    for (const auto &file : files) {
      //   int threads = omp_get_thread_num();
      // cout << "Open MP thread num " << threads << endl;

      std::string outputFilePath = output_dir;
      if (!output_dir.empty() && output_dir.back() != path_separator.back()) {
        outputFilePath += path_separator;
      }

      std::size_t lastSlash = file.find_last_of("/\\");
      std::size_t lastDot = file.find_last_of('.');
      std::string filename =
          file.substr(lastSlash + 1, lastDot - lastSlash - 1);
      outputFilePath += filename + ".txt";

      std::string converted = getImageText(file, lang);
      writeTextToFile(converted, outputFilePath);
    }
  }
};

#pragma endregion

/* Declaration Implementations */

#pragma region TEXT_SIMILARITY_IMPL

inline size_t levenshteinScore(std::string a, std::string b) {
  using namespace std;

  size_t m = a.size();
  size_t n = b.size();

  vector<vector<size_t>> dp(m + 1, vector<size_t>(n + 1));

  for (size_t i = 1; i < m; i++)
    dp[i][0] = i;

  for (size_t i = 1; i < n; i++)
    dp[0][i] = i;

  for (size_t i = 1; i <= m; i++) {
    for (size_t j = 1; j <= n; j++) {
      size_t eq = a[i - 1] == b[j - 1] ? 0 : 1;

      dp[i][j] = std::min(dp[i - 1][j - 1] + eq,
                          std::min(dp[i - 1][j] + 1, dp[i][j - 1] + 1));
    }
  }

  return dp[m][n];

  return 1;
}

#pragma endregion

#pragma region CRYPTOGRAPHY_IMPL
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <sstream>

inline std::string computeSHA256(const std::vector<uchar> &data) {
  EVP_MD_CTX *mdContext = EVP_MD_CTX_new();
  if (mdContext == nullptr) {
    throw std::runtime_error("Failed to create EVP_MD_CTX");
  }

  if (EVP_DigestInit_ex(mdContext, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(mdContext);
    throw std::runtime_error("Failed to initialize EVP Digest");
  }

  if (EVP_DigestUpdate(mdContext, data.data(), data.size()) != 1) {
    EVP_MD_CTX_free(mdContext);
    throw std::runtime_error("Failed to update digest");
  }

  unsigned char hash[EVP_MD_size(EVP_sha256())];
  unsigned int lengthOfHash = 0;

  if (EVP_DigestFinal_ex(mdContext, hash, &lengthOfHash) != 1) {
    EVP_MD_CTX_free(mdContext);
    throw std::runtime_error("Failed to finalize digest");
  }

  EVP_MD_CTX_free(mdContext);

  std::stringstream ss;
  for (unsigned int i = 0; i < lengthOfHash; ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return ss.str();
}

inline std::string computeSHA256(const std::string &filePath) {
  std::ifstream file(filePath, std::ifstream::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + filePath);
  }

  std::vector<uchar> data(std::istreambuf_iterator<char>(file), {});

  return computeSHA256(data);
}
#pragma endregion

#pragma region OPENCV_IMPL
inline std::string
extractTextFromImageBytes(const std::vector<uchar> &file_content,
                          const std::string &lang = "eng") {

  cv::Mat img = cv::imdecode(file_content, cv::IMREAD_COLOR);
  if (img.empty()) {
    throw std::runtime_error("Failed to load image from buffer");
  }

  cv::Mat gray;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
  cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

  tesseract::TessBaseAPI ocr;
  if (ocr.Init(nullptr, lang.c_str()) != 0) {
    throw std::runtime_error("Could not initialize tesseract.");
  }

  ocr.SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);
  std::string outText(ocr.GetUTF8Text());
  ocr.End();

  return outText;
}

inline std::string extractTextFromImageFile(const std::string &file_path,
                                            const std::string &lang) {

  cv::Mat img = cv::imread(file_path);
  if (img.empty()) {
    throw std::runtime_error("Failed to load image: " + file_path);
  }
  cv::Mat gray;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
  cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

  // Init Tesseract OCR engine
  // brew install tesseract-lang for all langs
  tesseract::TessBaseAPI ocr;
  if (ocr.Init(nullptr, lang.c_str()) != 0) {
    throw std::runtime_error("Could not initialize tesseract.");
  }

  // load image to Tesseract
  ocr.SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);

  // run OCR
  std::string outText(ocr.GetUTF8Text());

  ocr.End();

  return outText;
}

inline std::string extractTextFromImage(const std::string &file_path,
                                        ISOLang lang) {
  std::string langCode = isoToTesseractLang(lang);
  return extractTextFromImageFile(file_path, langCode);
}
#pragma endregion

#pragma region FILE_IO_IMPL
//  Read File : pass file path , get string
inline std::vector<uchar> readBytesFromFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uchar> buffer(size);
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    throw std::runtime_error("Failed to read file: " + filename);
  }
  return buffer;
}

// Write Content : write content to provided Path if it is a new file
inline void writeToNewFile(const std::string &content,
                           const std::string &output_path) {

  if (std::filesystem::exists(output_path)) {
    std::cerr << "Error: File already exists - " << output_path << std::endl;
    return;
  }
  std::ofstream outFile(output_path);
  if (!outFile) {
    std::cerr << "Error opening file: " << output_path << std::endl;
    return;
  }
  outFile << content;
}
#pragma endregion

#pragma region SYSTEM_IMPL      /* Helpers for Host Environment */
inline void printSystemInfo() {
#ifdef __clang__
  std::cout << "Clang version: " << __clang_version__ << std::endl;
#else
  std::cout << "Not using Clang." << std::endl;
#endif

#ifdef _OPENMP
  std::cout << "OpenMP is enabled." << std::endl;
#else
  std::cout << "OpenMP is not enabled." << std::endl;
#endif
};

#pragma endregion

#pragma region LOGGING_IMPL
enum class ANSICode {
  delimiter_star,
  delimiter_dim,
  green_bold,
  green,
  error,
  success_tick,
  failure_cross,
  warning_brightyellow,
  end,
};

constexpr const char *ANSI(ANSICode ansi) {
  switch (ansi) {
  case ANSICode::delimiter_dim:
    return "\x1b[90m***********************\x1b[0m";
  case ANSICode::delimiter_star:
    return "\x1b[90m***********************************************************"
           "********************\x1b[0m";
  case ANSICode::green:
    return "\x1b[92m";
  case ANSICode::green_bold:
    return "\x1b[1;32m";
  case ANSICode::error:
    return "\x1b[31m";
  case ANSICode::success_tick:
    return "\x1b[32m✔\x1b[0m";
  case ANSICode::failure_cross:
    return "\x1b[31m✖\x1b[0m";
  case ANSICode::warning_brightyellow:
    return "\x1b[93m";

  case ANSICode::end:
    return "\x1b[0m";
  default:
    return "";
  }
}
#pragma endregion

} // namespace imgstr

#endif // TEXTRACT_H
