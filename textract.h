
#ifndef TEXTRACT_H
#define TEXTRACT_H

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <folly/AtomicUnorderedMap.h>
#include <folly/SharedMutex.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <leptonica/allheaders.h>
#include <mutex>
#include <omp.h>
#include <openssl/evp.h>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>
#include <thread>
#include <unordered_set>
#include <vector>

namespace imgstr {

#pragma region TEXT_SIMILARITY            /* Text Similarity Declarations */

size_t levenshteinScore(std::string a, std::string b);

#pragma endregion

#pragma region CRYPTOGRAPHY               /* Cryptography Declarations */

std::string computeSHA256(const std::vector<unsigned char> &data);

std::string computeSHA256(const std::string &filePath);

#pragma endregion

#pragma region SYSTEM_UTILS               /* System Environment helpers */

enum class CORES { single, half, max };

enum class ISOLang { en, es, fr, hi, zh, de };

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

TimePoint getStartTime();

double getDuration(const TimePoint &start);

void printDuration(const TimePoint &startTime, const std::string &msg = "");

void printSystemInfo();

#define BOLD          "\x1b[1m"
#define ITALIC        "\x1b[3m"
#define UNDERLINE     "\x1b[4mT"
#define BRIGHT_WHITE  "\x1b[97m"
#define LIGHT_GREY    "\x1b[37m"
#define GREEN         "\x1b[92m"
#define BOLD_WHITE    "\x1b[1m"
#define CYAN          "\x1b[96m"
#define BLUE          "\x1b[94m"
#define GREEN_BOLD    "\x1b[1;32m"
#define ERROR         "\x1b[31m"
#define SUCCESS_TICK  "\x1b[32m✔\x1b[0m"
#define FAILURE_CROSS "\x1b[31m✖\x1b[0m"
#define WARNING       "\x1b[93m"
#define WARNING_BOLD  "\x1b[1;33m"
#define END           "\x1b[0m"
#define DELIMITER_STAR                                                         \
    "\x1b[90m***********************************************************\x1b[" \
    "0m"
#define DELIMITER_DIM                                                          \
    "\x1b[90m***********************************************************\x1b[" \
    "0m"
#define DELIMITER_ITEM                                                         \
    "------------------------------------------------------------------------" \
    "--"                                                                       \
    "----------"

inline std::string
isoToTesseractLang(ISOLang isoLang) {
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

#ifdef _WIN32
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

void createDirIfNotExists(const std::string &output_dir,
                          const char path_separator = SEPARATOR);

const std::string
createQualifiedFilePath(const std::string &input_path,
                        const std::string &output_dir,
                        const char path_separator = SEPARATOR);

std::vector<unsigned char> readBytesFromFile(const std::string &filename);

bool createFolder(const std::string &path);

unsigned long deleteFolder(const std::string &path);

void writeToNewFile(const std::string &content, const std::string &output_path);

bool isImageFile(const std::filesystem::path &path);

std::string getCurrentTimestamp();

#ifdef ENABLE_TIMING
static TimePoint startTime;
#define START_TIMING()  startTime = imgstr::getStartTime()
#define END_TIMING(msg) imgstr::printDuration(startTime, msg)
#else
#define START_TIMING()
#define END_TIMING(msg)
#endif

#pragma endregion

#pragma region OPENCV_UTILS               /* OpenCV Declarations */

enum class ImgMode { document, image };

std::string getTextOCR(const std::vector<unsigned char> &file_content,
                       const std::string &lang, ImgMode img_mode);

std::string getTextImgFile(const std::string &file_path,
                           const std::string &lang = "eng");

#ifndef DATAPATH
#define DATAPATH "/opt/homebrew/opt/tesseract/share/tessdata"
#endif

void createPDF(const std::string &input_path, const std::string &output_path,
               const char *datapath = DATAPATH);

std::string getTextOCRNoClear(const std::vector<unsigned char> &file_content,
                              const std::string &lang = "eng",
                              ImgMode img_mode = ImgMode::document);

#ifdef _USE_OPENCV

std::string getTextOCROpenCV(const std::vector<unsigned char> &file_content,
                             const std::string &lang);

std::string
extractTextFromImageBytes(const std::vector<unsigned char> &file_content,
                          const std::string &lang);

std::string extractTextFromImageFile(const std::string &file_path,
                                     const std::string &lang);

std::string extractTextFromImageFile(const std::string &file_path,
                                     ISOLang lang);
#endif

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
            cv.wait(lock,
                    [this] { return !log_queue.empty() || exit_flag.load(); });
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

    TesseractOCR() : ocrPtr(nullptr) {}

    void init(const std::string &lang = "eng",
              ImgMode mode = ImgMode::document) {
        if (!ocrPtr) {
            ocrPtr = new tesseract::TessBaseAPI();

            std::cerr << WARNING << "Created New Tesseract" << END << std::endl;
            TesseractThreadCount.fetch_add(1, std::memory_order_relaxed);

            if (ocrPtr->Init(nullptr, lang.c_str()) != 0) {
                delete ocrPtr;
                ocrPtr = nullptr;
                throw std::runtime_error("Could not initialize tesseract.");
            }
            if (mode == ImgMode::image) { /* Optimized for Complex Images */
                std::cout << "Image mode set" << '\n';
                ocrPtr->SetPageSegMode(tesseract::PSM_AUTO);
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

inline void
cleanupOpenMPTesserat() {

#pragma omp parallel
    {
        if (thread_local_tesserat.ocrPtr) {
            TesseractThreadCount.fetch_sub(1, std::memory_order_relaxed);
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
    ImgMode img_mode;
    CORES num_cores;
    std::string dir;
    std::vector<std::string> queued;
    std::unordered_set<std::string> files;
    std::unordered_set<std::string> processed;
    folly::AtomicUnorderedInsertMap<std::string, Image> cache;
    std::atomic<double> totalProcessingTime{0.0};
    std::atomic<int> processedImagesCount{0};

    std::unique_ptr<AsyncLogger> logger;

    const char path_separator = '/';
#ifdef _WIN32
    path_separator = '\\';
#endif

    std::optional<std::reference_wrapper<const Image>>
    processImageFile(const std::string &file) {

#ifdef _DEBUGAPP
        logger->log() << LIGHT_GREY << "processImageFile() for " << END << file;
#endif

        try {
            auto start = getStartTime();

            std::vector<unsigned char> data = readBytesFromFile(file);

            std::string img_hash = computeSHA256(data);

            auto img_from_cache = getFromCacheIfExists(img_hash);

            if (img_from_cache) {

                addProcessingTime(totalProcessingTime, getDuration(start));

                printCacheHit(file);

                return std::cref(img_from_cache->get());

            } else {

                std::string img_text = getTextOCR(data, "eng", img_mode);

                addProcessingTime(totalProcessingTime, getDuration(start));

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
    getImageOrProcess(const std::string &file_path,
                      ISOLang lang = ISOLang::en) {
        return processImageFile(file_path);
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

    void ifValidImageFileAppendQueue(const std::filesystem::path &path) {
        if (isImageFile(path)) {
            auto file_path = path.string();
            if (files.find(file_path) == files.end()) {
                files.insert(file_path);
                queued.emplace_back(file_path);
            }
        }
    }

    void addProcessingTime(std::atomic<double> &totalTime, double timeToAdd) {
        processedImagesCount++;
        double current = totalTime.load(std::memory_order_relaxed);
        double newTime;
        do {
            newTime = current + timeToAdd;
        } while (!totalTime.compare_exchange_weak(current, newTime,
                                                  std::memory_order_relaxed,
                                                  std::memory_order_relaxed));
    }

    double getAverageProcessingTime() {
        return totalProcessingTime.load() / processedImagesCount.load();
    }

    void initLog() {

#ifdef _DEBUGAPP
        logger->log() << ERROR << "DEBUG FLAGS ON\n" << END;
#endif

        logger->log() << "Processor Initialized" << '\n'
                      << "Threads Available: " << BOLD_WHITE
                      << omp_get_max_threads() << END
                      << "\nCores Available: " << BOLD_WHITE
                      << omp_get_num_procs() << END << '\n';
    }

    void printCacheHit(const std::string &file) {
        logger->log() << '\n'
                      << SUCCESS_TICK << GREEN << "  Cache Hit : " << END
                      << file << '\n';
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

    void fileOpenErrorLog(const std::string &output_path) {
        logger->log() << ERROR << "Error opening file: " << output_path;
    }

    void overWriteLog(const std::string &output_path) {
        logger->log() << WARNING_BOLD << "WARNING:  " << END << WARNING
                      << "File already exists - " << END << BOLD_WHITE
                      << output_path << END
                      << "    Are you sure you want to overwrite the file?"
                      << '\n';
        return;
    }

    void filesAlreadyProcessedLog() {
        logger->log() << BOLD_WHITE << "All files already processed." << END;
    }

    void printOutputAlreadyWritten(const Image &image) {
        logger->log() << DELIMITER_STAR << '\n'
                      << WARNING << image.getName()
                      << " Already Processed and written to " << END
                      << image.write_info.output_path << " at "
                      << image.write_info.write_timestamp << '\n';
    }

    void printProcessingFile(const std::string &file) {
        logger->log() << BOLD_WHITE << "Processing " << END << BRIGHT_WHITE
                      << file << END << '\n';
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
                  << BOLD_WHITE << "textract Processing Results\n\n"
                  << files.size() << END << " images proccessed\n"
                  << DELIMITER_STAR << '\n';

        for (const auto &img_sha : cache) {
            const Image &img = img_sha.second;

            logstream << GREEN_BOLD << std::left << std::setw(width)
                      << "SHA256: " << END << img.image_sha256 << '\n'
                      << BLUE << std::left << std::setw(width)
                      << "Path: " << END << img.path << '\n'
                      << BLUE << std::left << std::setw(width)
                      << "Image Size: " << END << img.image_size << " bytes\n"
                      << BLUE << std::left << std::setw(width)
                      << "Text Size: " << END << img.text_size << " bytes\n"
                      << BLUE << std::left << std::setw(width)
                      << "Processed Time: " << END << img.time_processed << '\n'
                      << BLUE << std::left << std::setw(width)
                      << "Output Path: " << END << img.write_info.output_path
                      << '\n'
                      << BLUE << std::left << std::setw(width)
                      << "Output Written: " << END
                      << (img.write_info.output_written ? "Yes" : "No") << '\n'
                      << BLUE << std::left << std::setw(width)
                      << "Write Timestamp: " << END
                      << img.write_info.write_timestamp << '\n'
                      << DELIMITER_ITEM << '\n';
        }

        logstream.flush();
    }

    void destructionLog() {
        logger->log() << LIGHT_GREY << "Destructor called - freeing "
                      << BRIGHT_WHITE
                      << imgstr::TesseractThreadCount.load(
                             std::memory_order_relaxed)
                      << END << " Tesseracts\n"
                      << END
                      << "\nAverage Image Processing Latency: " << BOLD_WHITE
                      << getAverageProcessingTime() << END << " ms\n"
                      << '\n'
                      <<

            BRIGHT_WHITE << "Total Images Processed : " << BOLD_WHITE
                      << processedImagesCount << END << '\n';
    }

  public:
    template <typename T>
    ImgProcessor(size_t capacity = 1000, T cores = 1) : ImgProcessor(capacity) {
        setCores(cores);
    }

    ImgProcessor(size_t capacity = 1000)
        : logger(std::make_unique<AsyncLogger>()), files(),
          cache(folly::AtomicUnorderedInsertMap<std::string, Image>(capacity)),
          img_mode(ImgMode::document) {

        initLog();

        setCores(1);
    }

    ~ImgProcessor() {

        destructionLog();

        cleanupOpenMPTesserat();
    }

    std::optional<std::string> getImageText(const std::string &file_path,
                                            ISOLang lang = ISOLang::en) {
        auto image = processImageFile(file_path);

        if (image)
            return image.value().get().text_content;

        return std::nullopt;
    }

    void processImagesDir(const std::string &directory,
                          bool write_output = false,
                          const std::string &output_path = "") {

        for (const auto &entry :
             std::filesystem::directory_iterator(directory)) {

            ifValidImageFileAppendQueue(entry.path());
        }

        if (write_output) {
            convertImagesToTextFilesParallel(output_path);
        }
    }

    void simpleProcessDir(const std::string &directory,
                          const std::string &output_path = "") {

        std::vector<std::string> filePaths;
        for (const auto &entry :
             std::filesystem::directory_iterator(directory)) {
            if (isImageFile(entry.path())) {
                filePaths.push_back(entry.path().string());
            }
        }

#pragma omp parallel for
        for (size_t i = 0; i < filePaths.size(); ++i) {
            START_TIMING();
            auto file_content = readBytesFromFile(filePaths[i]);
            auto img_text = getTextOCRNoClear(file_content);
            auto out_path = createQualifiedFilePath(filePaths[i], output_path);
            writeTextToFile(img_text, out_path);
            END_TIMING("simple: file processed and written ");
        }
    }

    void writeTextToFile(const std::string &content,
                         const std::string &output_path) {
        if (std::filesystem::exists(output_path)) {
            overWriteLog(output_path);
            return;
        }
        std::ofstream outFile(output_path);
        if (!outFile) {
            fileOpenErrorLog(output_path);
            return;
        }
        outFile << content;
    }

    void convertImageToTextFile(const std::string &input_file,
                                const std::string &output_dir = "",
                                ISOLang lang = ISOLang::en) {

        createDirIfNotExists(output_dir);

        std::string output_file =
            createQualifiedFilePath(input_file, output_dir);

        auto imageOpt = getImageOrProcess(input_file, lang);

        if (imageOpt) {

            const Image &image = imageOpt.value().get();

            if (!image.write_info.output_written) {

                writeTextToFile(image.text_content, output_file);
                image.updateWriteInfo(output_file, getCurrentTimestamp(), true);

            } else {
                printOutputAlreadyWritten(image);
            }
        }
    }

    void convertImagesToTextFilesParallel(const std::string &output_dir = "",
                                          ISOLang lang = ISOLang::en) {

        if (!output_dir.empty() && !std::filesystem::exists(output_dir))
            std::filesystem::create_directories(output_dir);

        if (queued.empty()) {
            filesAlreadyProcessedLog();
            return;
        }

#pragma omp parallel for
        for (const auto &file : queued) {
            START_TIMING();
            convertImageToTextFile(file, output_dir, lang);
            END_TIMING("parallel() - file processed ");
        }
        queued.clear();
    }

    void convertImagesToTextFiles(const std::string &output_dir = "",
                                  ISOLang lang = ISOLang::en) {

        createDirIfNotExists(output_dir);
        if (queued.empty()) {
            filesAlreadyProcessedLog();
            return;
        }

#pragma omp parallel for
        for (const auto &file : queued) {
            convertImageToTextFile(file, output_dir, lang);
        }
        queued.clear();
    }

    void generatePDF(const std::string &input_path,
                     const std::string &output_path,
                     const char *datapath = DATAPATH) {

        try {
            createPDF(input_path, output_path);

        } catch (const std::exception &e) {
            logger->log() << ERROR << "Failed to Generate PDF : " << e.what()
                          << END;
        }
    }

    /* utils */

    void setImageMode(ImgMode img_mode) { this->img_mode = img_mode; }

    template <typename T> void setCores(T cores) {
        if constexpr (std::is_same<T, CORES>::value) {
            switch (cores) {
            case CORES::single:
                omp_set_num_threads(1);
                break;
            case CORES::half:
                omp_set_num_threads(omp_get_num_procs() / 2);
                break;
            case CORES::max:
                omp_set_num_threads(omp_get_num_procs());
                break;
            }
        } else if constexpr (std::is_integral<T>::value) {
            int numThreads =
                std::min(static_cast<int>(cores), omp_get_num_procs());
            omp_set_num_threads(numThreads);
        } else {
            static_assert(false, "Unsupported type for setCores");
        }
    }

    void resetCache(size_t new_capacity) {
        cache =
            folly::AtomicUnorderedInsertMap<std::string, Image>(new_capacity);
    }

    template <typename Container> void addFiles(const Container &fileList) {
        for (const auto &file : fileList) {
            auto [_, inserted] = files.emplace(file);
            if (inserted) {
                queued.emplace_back(file);
            }
        }
    }

    template <typename... FileNames>
    std::vector<std::string> processImages(FileNames... fileNames) {
        addFiles({fileNames...});
        return processCurrentFiles();
    }

    void printFiles() {
        for (const auto &file : files) {
            logger->log() << file;
        }
    }

    void getResults() { printImagesInfo(); }
};

#pragma endregion

/* Implementations */

#pragma region TEXT_SIMILARITY_IMPL

inline size_t
levenshteinScore(std::string a, std::string b) {

    size_t m = a.size();
    size_t n = b.size();

    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));

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

inline std::string
computeSHA256(const std::vector<unsigned char> &data) {
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
        sha256 << std::hex << std::setw(2) << std::setfill('0')
               << (int) hash[i];
    }
    return sha256.str();
}

inline std::string
computeSHA256(const std::string &filePath) {
    std::ifstream file(filePath, std::ifstream::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }
    std::vector<unsigned char> data(std::istreambuf_iterator<char>(file), {});
    return computeSHA256(data);
}

#pragma endregion

#pragma region TESSERACT_OPENMP

#pragma region OPENCV_IMPL                /* OpenCV Image Processing Implementations */

/* valid image extensions */
static const std::unordered_set<std::string> validExtensions = {
    ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tif"};

inline bool
isImageFile(const std::filesystem::path &path) {
    return validExtensions.find(path.extension().string()) !=
           validExtensions.end();
}

inline void
tesseractInvokeLog(ImgMode img_mode) {
    int thread_id = omp_get_thread_num();
    std::cerr << ERROR << "getTextOCR "
              << (img_mode == ImgMode::document ? "document" : "image")
              << " mode " << END << " -> called from thread: " << thread_id
              << std::endl;
}

inline std::string
getTextOCR(const std::vector<unsigned char> &file_content,
           const std::string &lang, ImgMode img_mode = ImgMode::document) {

    /* Leptonica reads 40% or more faster than OpenCV */

#ifdef _DEBUGAPP
    tesseractInvokeLog(img_mode);
#endif

    if (!thread_local_tesserat.ocrPtr) {
        thread_local_tesserat.init(lang, img_mode);
    }
    Pix *image =
        pixReadMem(reinterpret_cast<const l_uint8 *>(file_content.data()),
                   file_content.size());
    if (!image)
        throw std::runtime_error("Failed to load image from memory buffer");

    thread_local_tesserat->SetImage(image);

    char *rawText = thread_local_tesserat->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;

    pixDestroy(&image);
    thread_local_tesserat->Clear();
    return outText;
};

inline void
createPDF(const std::string &input_path, const std::string &output_path,
          const char *datapath) {

    bool textonly = false;

    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    if (api->Init(datapath, "eng")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        delete api;
        return;
    }

    tesseract::TessPDFRenderer *renderer =
        new tesseract::TessPDFRenderer(output_path.c_str(), datapath, textonly);

    bool succeed = api->ProcessPages(input_path.c_str(), nullptr, 0, renderer);
    if (!succeed) {
        fprintf(stderr, "Error during processing.\n");
    }

    api->End();
    delete api;
    delete renderer;
}

inline std::string
getTextOCRNoClear(const std::vector<unsigned char> &file_content,
                  const std::string &lang, ImgMode img_mode) {

    if (!thread_local_tesserat.ocrPtr) {
        thread_local_tesserat.init(lang, img_mode);
    }
    Pix *image =
        pixReadMem(reinterpret_cast<const l_uint8 *>(file_content.data()),
                   file_content.size());
    if (!image)
        throw std::runtime_error("Failed to load image from memory buffer");

    thread_local_tesserat->SetImage(image);

    char *rawText = thread_local_tesserat->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;
    pixDestroy(&image);

    return outText;
};

inline std::string
getTextImgFile(const std::string &file_path, const std::string &lang) {

    if (!thread_local_tesserat.ocrPtr) {
        thread_local_tesserat.init(lang);
    }

    Pix *image = pixRead(file_path.c_str());

    thread_local_tesserat->SetImage(image);

    char *rawText = thread_local_tesserat->GetUTF8Text();
    std::string outText(rawText);

    delete[] rawText;

    pixDestroy(&image);
    thread_local_tesserat->Clear();
    return outText;
}

#ifdef _USE_OPENCV
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

#ifdef _USE_OPENCV

inline std::string
getTextOCROpenCV(const std::vector<uchar> &file_content,
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

inline std::string
extractTextFromImageFile(const std::string &file_path,
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

inline std::string
extractTextFromImage(const std::string &file_path, ISOLang lang) {
    std::string langCode = isoToTesseractLang(lang);
    return extractTextFromImageFile(file_path, langCode);
}
#endif

/* brew install tesseract-lang for all langs */

#pragma endregion

#pragma region FILE_IO_IMPL               /* STL File I/O Implementations */

inline void
createDirIfNotExists(const std::string &output_dir, const char path_separator) {

    if (!output_dir.empty() && !std::filesystem::exists(output_dir))
        std::filesystem::create_directories(output_dir);
}

inline const std::string
createQualifiedFilePath(const std::string &input_path,
                        const std::string &output_dir,
                        const char path_separator) {
    if (!output_dir.empty() && !std::filesystem::exists(output_dir))
        std::filesystem::create_directories(output_dir);

    std::string outputFilePath = output_dir;
    if (!output_dir.empty() && output_dir.back() != path_separator) {
        outputFilePath += path_separator;
    }

    std::size_t lastSlash = input_path.find_last_of("/\\");
    std::size_t lastDot = input_path.find_last_of('.');
    std::string filename =
        input_path.substr(lastSlash + 1, lastDot - lastSlash - 1);

    outputFilePath += filename + ".txt";

    return outputFilePath;
}

inline std::vector<unsigned char>
readBytesFromFile(const std::string &filename) {

#ifdef _DEBUGFILEIO
    std::cout << "Converting to char* " << filename << std::endl;
#endif

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

inline void
writeToNewFile(const std::string &content, const std::string &output_path) {

    if (std::filesystem::exists(output_path)) {
        std::cerr << "Error: File already exists - " << output_path
                  << std::endl;
        return;
    }
    std::ofstream outFile(output_path);
    if (!outFile) {
        std::cerr << "Error opening file: " << output_path << std::endl;
        return;
    }
    outFile << content;
}

inline std::string
getCurrentTimestamp() {
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

inline time_point
getStartTime() {
    return high_res_clock::now();
}

inline double
getDuration(const time_point &startTime) {
    auto endTime = high_res_clock::now();
    return std::chrono::duration<double>(endTime - startTime).count();
}

inline void
printDuration(const time_point &startTime, const std::string &msg) {
    auto endTime = high_res_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime);
    std::cout << BOLD << CYAN << msg << duration.count() << " ms" << END
              << std::endl;
}

inline void
printSystemInfo() {
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

}   // namespace imgstr

#endif   // TEXTRACT_H
