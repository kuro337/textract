
#ifndef TEXTRACT_H
#define TEXTRACT_H

#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <cstddef>
#include <folly/AtomicUnorderedMap.h>
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/Logger.h>
#include <folly/logging/xlog.h>
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

constexpr const char *ANSI(ANSICode ansi);

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

std::string getCurrentTimestamp();

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
  std::string image_sha256;
  std::string path;
  std::size_t image_size;
  std::string content_fuzzhash;
  std::string text_content;
  std::size_t text_size;
  std::string time_processed;
  std::string output_path;
  std::string write_timestamp;
  bool output_written = false;

  Image(const std::string &img_hash, const std::string &path,
        const std::string &text_content, size_t image_size = 0) {
    this->image_sha256 = img_hash;
    this->path = path;
    this->text_content = text_content;
    this->text_size = text_content.size();
    this->image_size = image_size;
    this->time_processed = getCurrentTimestamp();
  }

  std::string getName() const {
    auto lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
      return path.substr(lastSlash + 1);
    }
    return path;
  }
};

class ImgProcessor {

private:
  std::string dir;
  std::vector<std::string> files;
  folly::AtomicUnorderedInsertMap<std::string, Image> cache;

  std::optional<Image> processImageFile(const std::string &file) {
    try {
      std::vector<uchar> data = readBytesFromFile(file);
      std::string img_hash = computeSHA256(data);
      auto img_from_cache = getFromCacheIfExists(img_hash);

      if (img_from_cache) {

        std::cout << "processImageFile() got from cache hit" << '\n';

        printCacheHit(file);
        return img_from_cache;
      } else {

        std::string img_text = extractTextFromImageBytes(data, "eng");
        Image image(img_hash, file, img_text, data.size());
        Image img_cache = image;

        cache.emplace(img_hash, std::move(img_cache));
        return image;
      }
    } catch (const std::exception &e) {
      printFileProcessingFailure(file, e.what());
      return std::nullopt;
    }
  }

  std::optional<Image> getFromCacheIfExists(const std::string &img_sha) {
    auto img_from_cache = cache.find(img_sha);
    if (img_from_cache != cache.end()) {
      return img_from_cache->second;
    }
    return std::nullopt;
  }

  std::vector<std::string> processCurrentFiles() {

    if (files.empty()) {
      std::cout << "Files are empty" << std::endl;
      return {};
    }

    std::vector<std::string> processedText;

    for (const auto &file : files) {
      auto image = processImageFile(file);
      if (image)
        processedText.emplace_back(image.value().text_content);
    }

    return processedText;
  }

  void printCacheHit(const std::string &file) {

    std::cout << delimiter << '\n'
              << check_mark << green << "  Image Already Processed : " << end
              << file << '\n';
  }

  void printOutputAlreadyWritten(const Image &img) {

    std::cout << delimiter << '\n'
              << ANSI(ANSICode::warning_brightyellow) << img.getName()
              << " Already Processed and written to " << end << img.output_path
              << " at " << img.write_timestamp << '\n';
  }

  void printFileProcessingFailure(const std::string &file,
                                  const std::string &err_msg) {
    std::cout << "Failed to Extract Text from Image file: " << file
              << ". Error: " << err_msg << '\n';
  }

  void printFiles() {
    for (const auto &file : files) {
      std::cout << file << std::endl;
    }
  }

  void printImagesInfo() {
    std::cout << ANSI(ANSICode::delimiter_star) << '\n';
    for (const auto &img_sha : cache) {
      const Image &img = img_sha.second;
      std::cout << ANSI(ANSICode::green_bold)
                << "Image SHA256: " << ANSI(ANSICode::end) << img.image_sha256
                << '\n';
      std::cout << ANSI(ANSICode::green) << "Path: " << ANSI(ANSICode::end)
                << img.path << '\n';
      std::cout << ANSI(ANSICode::green)
                << "Image Size: " << ANSI(ANSICode::end) << img.image_size
                << " bytes\n";
      std::cout << ANSI(ANSICode::green) << "Text Size: " << ANSI(ANSICode::end)
                << img.text_size << " bytes\n";
      std::cout << ANSI(ANSICode::green)
                << "Processed Time: " << ANSI(ANSICode::end)
                << img.time_processed << '\n';
      std::cout << ANSI(ANSICode::green)
                << "Output Path: " << ANSI(ANSICode::end) << img.output_path
                << '\n';
      std::cout << ANSI(ANSICode::green)
                << "Output Written: " << ANSI(ANSICode::end)
                << (img.output_written ? "Yes" : "No") << '\n';
      std::cout << ANSI(ANSICode::green)
                << "Write Timestamp: " << ANSI(ANSICode::end)
                << img.write_timestamp << "\n\n";
    }
    std::cout << ANSI(ANSICode::delimiter_star) << '\n';
  }

public:
  ImgProcessor(size_t capacity = 1000)
      : files(),
        cache(folly::AtomicUnorderedInsertMap<std::string, Image>(capacity)) {}

  void addFile(const std::string &file_path) { files.push_back(file_path); }

  void resetCache(size_t new_capacity) {
    cache = folly::AtomicUnorderedInsertMap<std::string, Image>(new_capacity);
  }

  template <typename... FileNames>
  std::vector<std::string> processImages(FileNames... fileNames) {
    addFiles({fileNames...});

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

  std::optional<Image> getImage(const std::string &file_path,
                                ISOLang lang = ISOLang::en) {
    auto img = processImageFile(file_path);
    if (img) {

      return img.value();
    }
    return std::nullopt;
  }

  std::optional<std::string> getImageText(const std::string &file_path,
                                          ISOLang lang = ISOLang::en) {
    auto image = processImageFile(file_path);
    if (image)
      return image.value().text_content;

    return std::nullopt;
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

    if (!output_dir.empty() && !std::filesystem::exists(output_dir))
      std::filesystem::create_directories(output_dir);

    std::string outputFilePath = output_dir;
    if (!output_dir.empty() && output_dir.back() != path_separator.back()) {
      outputFilePath += path_separator;
    }

    std::size_t lastSlash = input_file.find_last_of("/\\");
    std::size_t lastDot = input_file.find_last_of('.');
    std::string filename =
        input_file.substr(lastSlash + 1, lastDot - lastSlash - 1);
    outputFilePath += filename + ".txt";

    std::cout << "output path is " << outputFilePath << '\n';

    auto imageOpt = getImage(input_file, lang);

    if (imageOpt) {

      auto image = imageOpt.value();

      if (!image.output_written) {
        std::cout << "writing img first time" << '\n';

        /* need to use a shared mutex for updates */

        writeTextToFile(image.text_content, outputFilePath);
        image.output_written = true;
        image.output_path = outputFilePath;
        image.write_timestamp = getCurrentTimestamp();
        cache.emplace(image.image_sha256, std::move(image));
      } else {
        printOutputAlreadyWritten(image);
      }
    }
  }

  void convertImagesToTextFiles(const std::string &output_dir = "",
                                ISOLang lang = ISOLang::en) {
#ifdef _WIN32
    std::string path_separator = "\\";
#else
    std::string path_separator = "/";
#endif

    if (!output_dir.empty() && !std::filesystem::exists(output_dir))
      std::filesystem::create_directories(output_dir);

#pragma omp parallel for
    for (const auto &file : files) {

      std::cout << "Procesing " << file << '\n';

      convertImageToTextFile(file, output_dir, lang);
    }
  }

  void getResults() { printImagesInfo(); }
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

inline std::string getCurrentTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
  return ss.str();
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
