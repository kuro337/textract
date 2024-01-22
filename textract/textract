
#ifndef TEXTRACT_H
#define TEXTRACT_H

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <folly/AtomicUnorderedMap.h>
#include <folly/SharedMutex.h>
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/logging/Logger.h>
#include <folly/logging/xlog.h>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <leptonica/allheaders.h>
#include <mutex>
#include <omp.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <openssl/evp.h>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <tesseract/baseapi.h>
#include <thread>
#include <unordered_set>
#include <vector>

namespace imgstr {

#pragma region TEXT_SIMILARITY            /* Text Similarity Declarations */

size_t levenshteinScore(std::string a, std::string b);

#pragma endregion

#pragma region CRYPTOGRAPHY               /* Cryptography Declarations */

std::string computeSHA256(const std::vector<uchar> &data);

std::string computeSHA256(const std::string &filePath);

#pragma endregion

#pragma region SYSTEM_UTILS               /* System Environment helpers */

enum class CORES { single, half, max };

enum class ISOLang { en, es, fr, hi, zh, de };

std::chrono::time_point<std::chrono::high_resolution_clock> getStartTime();

double getDuration(
    const std::chrono::time_point<std::chrono::high_resolution_clock> &start);

void printSystemInfo();

#define BOLD "\x1b[1m"
#define ITALIC "\x1b[3m"
#define UNDERLINE "\x1b[4mT"
#define BRIGHT_WHITE "\x1b[97m"
#define LIGHT_GREY "\x1b[37m"
#define GREEN "\x1b[92m"
#define BOLD_WHITE "\x1b[1m"
#define CYAN "\x1b[96m"
#define BLUE "\x1b[94m"
#define GREEN_BOLD "\x1b[1;32m"
#define ERROR "\x1b[31m"
#define SUCCESS_TICK "\x1b[32m✔\x1b[0m"
#define FAILURE_CROSS "\x1b[31m✖\x1b[0m"
#define WARNING "\x1b[93m"
#define WARNING_BOLD "\x1b[1;33m"
#define END "\x1b[0m"
#define DELIMITER_STAR                                                         \
  "\x1b[90m***********************************************************\x1b[0m"
#define DELIMITER_DIM                                                          \
  "\x1b[90m***********************************************************\x1b[0m"
#define DELIMITER_ITEM                                                         \
  "--------------------------------------------------------------------------" \
  "----------"

inline constexpr std::string isoToTesseractLang(ISOLang isoLang) {
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

#pragma endregion

#pragma region FILE_IO                    /* File IO Declarations */

std::vector<uchar> readBytesFromFile(const std::string &filename);

void writeToNewFile(const std::string &content, const std::string &output_path);

bool isImageFile(const std::filesystem::path &path);

std::string getCurrentTimestamp();

#pragma endregion

#pragma region OPENCV_UTILS               /* OpenCV Declarations */

std::string getTextOCR(const std::vector<uchar> &file_content,
                       const std::string &lang);

std::string getTextOCROpenCV(const std::vector<uchar> &file_content,
                             const std::string &lang);

std::string extractTextFromImageBytes(const std::vector<uchar> &file_content,
                                      const std::string &lang);

std::string extractTextFromImageFile(const std::string &file_path,
                                     const std::string &lang);

std::string extractTextFromImageFile(const std::string &file_path,
                                     ISOLang lang);

void cleanupOpenMPTesserat();

#pragma endregion

#pragma region ASYNC_LOGGER      /* Non Blocking Asynchronous Logger with ANSI Escaping */

class AsyncLogger {
public:
  AsyncLogger() : exit_flag(false) {
    worker_thread = std::thread(&AsyncLogger::processEntries, this);
  }

  ~AsyncLogger() {
    exit_flag.store(true);
    cv.notify_one();
    worker_thread.join();
  }

  class LogStream {
  public:
    LogStream(AsyncLogger &logger, bool immediateFlush = true)
        : logger(logger), immediateFlush(immediateFlush) {}

    ~LogStream() { logger.log(stream.str()); }

    template <typename T> LogStream &operator<<(const T &msg) {
      stream << msg;
      return *this;
    }

    void flush() {
      if (!immediateFlush) {
        logger.log(stream.str());
        stream.str("");
        stream.clear();
      }
    }

  private:
    AsyncLogger &logger;
    std::ostringstream stream;
    bool immediateFlush;
  };

  LogStream log() { return LogStream(*this); }
  LogStream stream() { return LogStream(*this, false); }

private:
  std::thread worker_thread;
  std::mutex queue_mutex;
  std::condition_variable cv;
  std::queue<std::string> log_queue;
  std::atomic<bool> exit_flag;

  void log(const std::string &message) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    log_queue.push(message);
    cv.notify_one();
  }

  void processEntries() {
    std::unique_lock<std::mutex> lock(queue_mutex, std::defer_lock);
    while (true) {
      lock.lock();
      cv.wait(lock, [this] { return !log_queue.empty() || exit_flag.load(); });
      while (!log_queue.empty()) {
        std::cout << log_queue.front() << std::endl;
        log_queue.pop();
      }
      if (exit_flag.load())
        break;
      lock.unlock();
    }
  }
};

#pragma endregion

#pragma region TESSERACT_OPENMP          /* Tesseract Implementation for Thread Local Tesseracts'  */

static std::atomic<int> TesseractThreadCount(0);

struct TesseractOCR {
  tesseract::TessBaseAPI *ocrPtr;

  TesseractOCR() : ocrPtr(nullptr) {
    TesseractThreadCount.fetch_add(1, std::memory_order_relaxed);
  }

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

  ~TesseractOCR() {
    if (ocrPtr) {
      ocrPtr->Clear();
      ocrPtr->End();
      delete ocrPtr;
    }
  }

  tesseract::TessBaseAPI *operator->() const { return ocrPtr; }
};

static TesseractOCR thread_local_tesserat;

#pragma omp threadprivate(thread_local_tesserat)

inline void cleanupOpenMPTesserat() {
  int num_cores = omp_get_num_procs();
  omp_set_num_threads(num_cores);
#pragma omp parallel
  {
    if (thread_local_tesserat.ocrPtr) {
      thread_local_tesserat.~TesseractOCR();
      thread_local_tesserat.ocrPtr = nullptr;
    }
  }
}

#pragma endregion

/*

ImgProcessor : Core header class

Provides an efficient, high performance implementation
of Text Extraction from Images.

Supports Parallelized Image Processing and maintains an in-memory cache.

Uses an Atomic Unordered Map for Safe Wait-Free parallel access

Cache retrieval logic is determined by the SHA256 hash of the Image bytes

The SHA256 Byte Hash enables duplicate images to not be processed even if the
file names or paths differ.

*/

#pragma region imgstr_core      /* Core Class for Image Processing and Text Extraction */

struct WriteMetadata {
  std::string output_path;
  std::string write_timestamp;
  bool output_written = false;
};

struct Image {
  std::string image_sha256;
  std::string path;
  std::size_t image_size;
  std::string content_fuzzhash;
  std::string text_content;
  std::size_t text_size;
  std::string time_processed;
  mutable WriteMetadata write_info;
  mutable std::unique_ptr<folly::SharedMutex> mutex;

  Image(const std::string &img_hash, const std::string &path,
        const std::string &text_content, size_t image_size = 0) {
    this->image_sha256 = img_hash;
    this->path = path;
    this->text_content = text_content;
    this->text_size = text_content.size();
    this->image_size = image_size;
    this->time_processed = getCurrentTimestamp();
    this->mutex = nullptr;
  }

  void updateWriteInfo(const std::string &output_path,
                       const std::string &write_timestamp,
                       bool output_written) const {
    if (!mutex) {
      mutex = std::make_unique<folly::SharedMutex>();
    }
    std::unique_lock<folly::SharedMutex> writerLock(*mutex);
    write_info.output_path = output_path;
    write_info.write_timestamp = write_timestamp;
    write_info.output_written = output_written;
  }

  WriteMetadata readWriteInfoSafe() {
    if (!mutex) {
      return write_info;
    }
    std::shared_lock<folly::SharedMutex> readerLock(*mutex);
    return write_info;
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
  std::vector<std::string> queued;
  std::unordered_set<std::string> files;
  std::unordered_set<std::string> processed;
  folly::AtomicUnorderedInsertMap<std::string, Image> cache;
  std::unique_ptr<AsyncLogger> logger;

  std::optional<std::reference_wrapper<const Image>>
  processImageFile(const std::string &file) {
    try {
      std::vector<uchar> data = readBytesFromFile(file);
      std::string img_hash = computeSHA256(data);

      auto img_from_cache = getFromCacheIfExists(img_hash);
      if (img_from_cache) {

        printCacheHit(file);
        return std::cref(img_from_cache->get());
      } else {

        std::string img_text = getTextOCR(data, "eng");
        Image image(img_hash, file, img_text, data.size());

        auto &cachedImage =
            cache.emplace(img_hash, std::move(image)).first->second;

        processed.insert(file);

        return std::cref(cachedImage);
      }
    } catch (const std::exception &e) {
      printFileProcessingFailure(file, e.what());
      return std::nullopt;
    }
  }

  std::optional<std::reference_wrapper<const Image>>
  getFromCacheIfExists(const std::string &img_sha) {
    auto img_from_cache = cache.find(img_sha);
    if (img_from_cache != cache.end()) {
      return std::cref(img_from_cache->second);
    }
    return std::nullopt;
  }

  std::vector<std::string> processCurrentFiles() {
    if (files.empty()) {
      logger->log() << "Files are empty";
      return {};
    }

    std::vector<std::string> processedText;

    for (const auto &file : files) {
      auto image = processImageFile(file);
      if (image)
        processedText.emplace_back(image.value().get().text_content);
    }

    return processedText;
  }

  void printCacheHit(const std::string &file) {
    logger->log() << '\n'
                  << SUCCESS_TICK << GREEN << "  Cache Hit : " << END << file
                  << '\n';
  }

  void printFileProcessingFailure(const std::string &file,
                                  const std::string &err_msg) {
    logger->log() << "Failed to Extract Text from Image file: " << file
                  << ". Error: " << err_msg << '\n';
  }

  void printInputFileAlreadyProcessed(const std::string &file) {
    logger->log() << DELIMITER_STAR << '\n'
                  << WARNING << "File at path : " << END << file
                  << "has already been processed to text" << '\n';
  }

  void printOutputAlreadyWritten(const Image &image) {
    logger->log() << DELIMITER_STAR << '\n'
                  << WARNING << image.getName()
                  << " Already Processed and written to " << END
                  << image.write_info.output_path << " at "
                  << image.write_info.write_timestamp << '\n';
  }

  void printProcessingFile(const std::string &file) {
    logger->log() << BOLD_WHITE << "Processing " << END << BRIGHT_WHITE << file
                  << END << '\n';
  }

  void printProcessingDuration(double duration_ms) {

    logger->log() << DELIMITER_STAR << '\n'
                  << BOLD_WHITE << queued.size() << END
                  << " Files Processed and Converted in " << BRIGHT_WHITE
                  << duration_ms << " seconds\n"
                  << END << DELIMITER_STAR << "\n";
  }

  void printImagesInfo() {
    const int width = 20;

    auto logstream = logger->stream();

    logstream << DELIMITER_STAR << '\n'
              << BOLD_WHITE << "textract Processing Results\n"
              << END << DELIMITER_STAR << '\n';

    for (const auto &img_sha : cache) {
      const Image &img = img_sha.second;

      logstream << GREEN_BOLD << std::left << std::setw(width)
                << "SHA256: " << END << img.image_sha256 << '\n'
                << BLUE << std::left << std::setw(width) << "Path: " << END
                << img.path << '\n'
                << BLUE << std::left << std::setw(width)
                << "Image Size: " << END << img.image_size << " bytes\n"
                << BLUE << std::left << std::setw(width) << "Text Size: " << END
                << img.text_size << " bytes\n"
                << BLUE << std::left << std::setw(width)
                << "Processed Time: " << END << img.time_processed << '\n'
                << BLUE << std::left << std::setw(width)
                << "Output Path: " << END << img.write_info.output_path << '\n'
                << BLUE << std::left << std::setw(width)
                << "Output Written: " << END
                << (img.write_info.output_written ? "Yes" : "No") << '\n'
                << BLUE << std::left << std::setw(width)
                << "Write Timestamp: " << END << img.write_info.write_timestamp
                << '\n'
                << DELIMITER_ITEM << '\n';
    }

    logstream.flush();
  }

public:
  ImgProcessor(size_t capacity = 1000)
      : logger(std::make_unique<AsyncLogger>()), files(),
        cache(folly::AtomicUnorderedInsertMap<std::string, Image>(capacity)) {
    logger->log() << "Processor Initialized";
  }

  ~ImgProcessor() {

    logger->log() << LIGHT_GREY << "Destructor called - freeing "
                  << BRIGHT_WHITE
                  << imgstr::TesseractThreadCount.load(
                         std::memory_order_relaxed)
                  << END << " Tesseracts\n"
                  << END;

    cleanupOpenMPTesserat();
  }

  void addFile(const std::string &file_path) {
    if (files.find(file_path) != files.end()) {
      files.insert(file_path);
      queued.emplace_back(file_path);
    }
  }

  void resetCache(size_t new_capacity) {
    cache = folly::AtomicUnorderedInsertMap<std::string, Image>(new_capacity);
  }

  void processImagesDir(const std::string &directory, bool write_output = false,
                        const std::string &output_path = "") {
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
      if (isImageFile(entry.path())) {

        auto file_path = entry.path().string();

        if (files.find(file_path) == files.end()) {
          files.insert(file_path);
          queued.emplace_back(file_path);
        }
      }
    }

    if (write_output) {
      convertImagesToTextFiles(output_path);
    }
  }

  template <typename... FileNames>
  std::vector<std::string> processImages(FileNames... fileNames) {
    addFiles({fileNames...});
    return processCurrentFiles();
  }

  void addFiles(std::initializer_list<std::string> fileList) {
    for (const auto &file : fileList) {

      if (files.find(file) == files.end()) {
        files.insert(file);
        queued.emplace_back(file);
      }
    }
  }

  void addFiles(const std::vector<std::string> &fileList) {
    for (const auto &file : fileList) {

      if (files.find(file) == files.end()) {
        files.insert(file);
        queued.emplace_back(file);
      }
    }
  }

  std::optional<std::reference_wrapper<const Image>>
  getImageOrProcess(const std::string &file_path, ISOLang lang = ISOLang::en) {
    return processImageFile(file_path);
  }

  std::optional<std::string> getImageText(const std::string &file_path,
                                          ISOLang lang = ISOLang::en) {
    auto image = processImageFile(file_path);

    if (image)
      return image.value().get().text_content;

    return std::nullopt;
  }

  void writeTextToFile(const std::string &content,
                       const std::string &output_path) {

    if (std::filesystem::exists(output_path)) {
      logger->log() << WARNING_BOLD << "WARNING:  " << END << WARNING
                    << "File already exists - " << END << BOLD_WHITE
                    << output_path << END
                    << "    Are you sure you want to overwrite the file?"
                    << '\n';
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

    printProcessingFile(input_file);

    std::string outputFilePath = output_dir;
    if (!output_dir.empty() && output_dir.back() != path_separator.back()) {
      outputFilePath += path_separator;
    }

    std::size_t lastSlash = input_file.find_last_of("/\\");
    std::size_t lastDot = input_file.find_last_of('.');
    std::string filename =
        input_file.substr(lastSlash + 1, lastDot - lastSlash - 1);
    outputFilePath += filename + ".txt";

    auto imageOpt = getImageOrProcess(input_file, lang);

    if (imageOpt) {
      const Image &image = imageOpt.value().get();

      if (!image.write_info.output_written) {
        writeTextToFile(image.text_content, outputFilePath);

        image.updateWriteInfo(outputFilePath, getCurrentTimestamp(), true);
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

    auto start = getStartTime();

    if (queued.empty()) {
      logger->log() << BOLD_WHITE << "All files have already been processed."
                    << END;
      return;
    }

#pragma omp parallel for
    for (const auto &file : queued) {
      convertImageToTextFile(file, output_dir, lang);
    }

    printProcessingDuration(getDuration(start));
    queued.clear();
  }

  void getResults() { printImagesInfo(); }

  void printFiles() {
    for (const auto &file : files) {
      logger->log() << file;
    }
  }
};

#pragma endregion

/* Implementations */

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
}

#pragma endregion

#pragma region CRYPTOGRAPHY_IMPL           /*    OpenSSL4 Cryptography Implementations    */

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

  std::stringstream sha256;
  for (unsigned int i = 0; i < lengthOfHash; ++i) {
    sha256 << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return sha256.str();
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

#pragma region TESSERACT_OPENMP

#pragma region OPENCV_IMPL                /* OpenCV Image Processing Implementations */

/* valid image extensions */
static const std::unordered_set<std::string> validExtensions = {
    ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tif"};

inline bool isImageFile(const std::filesystem::path &path) {
  return validExtensions.find(path.extension().string()) !=
         validExtensions.end();
}

inline std::string getTextOCR(const std::vector<uchar> &file_content,
                              const std::string &lang) {

  /* Leptonica reads 40% or more faster than OpenCV */

  if (!thread_local_tesserat.ocrPtr) {
    std::cout << "New Tesserat instance created\n" << std::endl;
    thread_local_tesserat.init(lang);
  }
  Pix *image =
      pixReadMem(reinterpret_cast<const l_uint8 *>(file_content.data()),
                 file_content.size());
  if (!image)
    throw std::runtime_error("Failed to load image from memory buffer");

  thread_local_tesserat->SetImage(image);
  std::string outText(thread_local_tesserat->GetUTF8Text());

  pixDestroy(&image);

  return outText;
};

inline std::string getTextOCROpenCV(const std::vector<uchar> &file_content,
                                    const std::string &lang) {

  if (!thread_local_tesserat.ocrPtr) {
    std::cout << "New Tesserat instance created\n" << std::endl;
    thread_local_tesserat.init(lang);
  }

  cv::Mat img = cv::imdecode(file_content, cv::IMREAD_COLOR);
  if (img.empty()) {
    throw std::runtime_error("Failed to load image from buffer");
  }

  cv::Mat gray;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
  cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

  thread_local_tesserat->SetImage(gray.data, gray.cols, gray.rows, 1,
                                  gray.step);
  std::string outText(thread_local_tesserat->GetUTF8Text());

  return outText;
};

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

  tesseract::TessBaseAPI ocr;
  if (ocr.Init(nullptr, lang.c_str()) != 0) {
    throw std::runtime_error("Could not initialize tesseract.");
  }

  ocr.SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);

  std::string outText(ocr.GetUTF8Text());

  ocr.End();

  return outText;
}

inline std::string extractTextFromImage(const std::string &file_path,
                                        ISOLang lang) {
  std::string langCode = isoToTesseractLang(lang);
  return extractTextFromImageFile(file_path, langCode);
}

/* brew install tesseract-lang for all langs */

#pragma endregion

#pragma region FILE_IO_IMPL               /* STL File I/O Implementations */

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

#pragma region SYSTEM_IMPL                /* System Environment Util Implementations */

using high_res_clock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<high_res_clock>;

inline time_point getStartTime() { return high_res_clock::now(); }

inline double getDuration(const time_point &startTime) {
  auto endTime = high_res_clock::now();
  return std::chrono::duration<double>(endTime - startTime).count();
}

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

} // namespace imgstr

#endif // TEXTRACT_H
