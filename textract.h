#ifndef TEXTRACT_H
#define TEXTRACT_H

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <folly/AtomicUnorderedMap.h>
#include <folly/SharedMutex.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <leptonica/allheaders.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <mutex>
#include <omp.h>
#include <openssl/evp.h>
#include <optional>
#include <queue>
#include <src/fs.h>
#include <sstream>
#include <string>
#include <tesseract/baseapi.h>
#include <tesseract/renderer.h>
#include <thread>
#include <unordered_set>
#include <util.h>
#include <utility>
#include <vector>

namespace imgstr {

#pragma region TEXT_SIMILARITY            /* Text Similarity Declarations */

    auto levenshteinScore(std::string a, std::string b) -> size_t;

#pragma endregion

#pragma region CRYPTOGRAPHY               /* Cryptography Declarations */

    auto computeSHA256(const std::vector<unsigned char> &data) -> std::string;

    auto computeSHA256(const std::string &filePath) -> std::string;

#pragma endregion

#pragma region SYSTEM_UTILS               /* System Environment helpers */

    enum class CORES { single, half, max };

    enum class ISOLang { en, es, fr, hi, zh, de };

    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    auto getStartTime() -> TimePoint;

    auto getDuration(const TimePoint &start) -> double;

    void printDuration(const TimePoint &startTime, const std::string &msg = "");

    void printSystemInfo();

    namespace Ansi {
        static constexpr auto BOLD          = "\x1b[1m";
        static constexpr auto ITALIC        = "\x1b[3m";
        static constexpr auto UNDERLINE     = "\x1b[4mT";
        static constexpr auto BRIGHT_WHITE  = "\x1b[97m";
        static constexpr auto LIGHT_GREY    = "\x1b[37m";
        static constexpr auto GREEN         = "\x1b[92m";
        static constexpr auto BOLD_WHITE    = "\x1b[1m";
        static constexpr auto CYAN          = "\x1b[96m";
        static constexpr auto BLUE          = "\x1b[94m";
        static constexpr auto GREEN_BOLD    = "\x1b[1;32m";
        static constexpr auto ERROR         = "\x1b[31m";
        static constexpr auto SUCCESS_TICK  = "\x1b[32m✔\x1b[0m";
        static constexpr auto FAILURE_CROSS = "\x1b[31m✖\x1b[0m";
        static constexpr auto WARNING       = "\x1b[93m";
        static constexpr auto WARNING_BOLD  = "\x1b[1;33m";
        static constexpr auto END           = "\x1b[0m";
        static constexpr auto DELIMITER_STAR =
            "\x1b[90m******************************************************"
            "\x1b[";
        static constexpr auto DELIMITER_DIM =
            "\x1b[90m******************************************************"
            "\x1b[";
        static constexpr auto DELIMITER_ITEM =
            "--------------------------------------------------------------";
    } // namespace Ansi

    using namespace Ansi;

    namespace ISOLanguage {
        static constexpr auto eng = "eng";
        static constexpr auto esp = "spa";
        static constexpr auto fra = "fra";
        static constexpr auto ger = "deu";
        static constexpr auto chi = "chi_sim";
        static constexpr auto hin = "hin";
    } // namespace ISOLanguage

#pragma endregion

#pragma region FILE_IO                    /* File IO Declarations */

#ifdef _WIN32
    static constexpr char SEPARATOR = '\\';
#else
    static constexpr char SEPARATOR = '/';
#endif

    auto fileExists(const std::string &path) -> bool;

    auto readBytesFromFile(const std::string &filename) -> std::vector<unsigned char>;

    auto createFolder(const std::string &path) -> bool;

    auto deleteFolder(const std::string &path) -> unsigned long;

    void writeToNewFile(const std::string &content, const std::string &output_path);

    auto isImageFile(const llvm::StringRef &path) -> bool;

    auto getCurrentTimestamp() -> std::string;

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

    auto getTextOCR(const std::vector<unsigned char> &file_content,
                    const std::string                &lang,
                    ImgMode                           img_mode) -> std::string;

    auto getTextImgFile(const std::string &file_path,
                        const std::string &lang = "eng") -> std::string;

#ifndef DATAPATH
    static constexpr auto DATAPATH = "/opt/homebrew/opt/tesseract/share/tessdata";
#endif

    void createPDF(const std::string &input_path,
                   const std::string &output_path,
                   const char        *datapath = DATAPATH);

    auto getTextOCRNoClear(const std::vector<unsigned char> &file_content,
                           const std::string                &lang = "eng",
                           ImgMode img_mode = ImgMode::document) -> std::string;

#ifdef _USE_OPENCV

    std::string
        getTextOCROpenCV(const std::vector<unsigned char> &file_content, const std::string &lang);

    std::string extractTextFromImageBytes(const std::vector<unsigned char> &file_content,
                                          const std::string                &lang);

    std::string extractTextFromImageFile(const std::string &file_path, const std::string &lang);

    std::string extractTextFromImageFile(const std::string &file_path, ISOLang lang);
#endif

    void cleanupOpenMPTesserat();

    void logThreadUse();

#pragma endregion

    inline auto &sout = llvm::outs();
    inline auto &serr = llvm::errs();

#pragma region ASYNC_LOGGER      /* Non Blocking Asynchronous Logger with ANSI Escaping */

    class AsyncLogger {
      public:
        AsyncLogger(): exit_flag(false) {
            worker_thread = std::thread(&AsyncLogger::processEntries, this);
        }

        AsyncLogger(const AsyncLogger &)                     = delete;
        AsyncLogger(AsyncLogger &&)                          = delete;
        auto operator=(const AsyncLogger &) -> AsyncLogger & = delete;
        auto operator=(AsyncLogger &&) -> AsyncLogger      & = delete;

        ~AsyncLogger() {
            exit_flag.store(true);
            cv.notify_one();
            worker_thread.join();
        }

        class LogStream {
          public:
            LogStream(AsyncLogger &logger, bool immediateFlush = true)
                : logger(logger),
                  immediateFlush(immediateFlush) {}

            ~LogStream() {
                if (!error) {
                    try {
                        logger.log(stream.str());
                    } catch (const std::exception &e) {
                        serr << "Exception thrown during Logging: " << e.what() << '\n';
                    } catch (...) {
                        serr << "Logging failed during stream destruction.\n";
                    }
                }
            }

            LogStream(const LogStream &)                     = delete;
            LogStream(LogStream &&)                          = delete;
            auto operator=(const LogStream &) -> LogStream & = delete;
            auto operator=(LogStream &&) -> LogStream      & = delete;

            template <typename T>
            auto operator<<(const T &msg) -> LogStream & {
                stream << msg;
                return *this;
            }

            void flush() {
                if (!immediateFlush && !error) {
                    try {
                        logger.log(stream.str());
                        stream.str("");
                        stream.clear();
                    } catch (const std::exception &e) {
                        serr << "Exception thrown during flush: " << e.what() << '\n';
                    } catch (...) {
                        serr << "Failed to flush logs.\n";
                        error = true;
                    }
                }
            }

          private:
            AsyncLogger       &logger;
            std::ostringstream stream;
            bool               immediateFlush;
            bool               error {};
        };

        auto log() -> LogStream { return (*this); }

        auto stream() -> LogStream { return {*this, false}; }

      private:
        std::thread             worker_thread;
        std::mutex              queue_mutex;
        std::condition_variable cv;
        std::queue<std::string> log_queue;
        std::atomic<bool>       exit_flag;

        void log(const std::string &message) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            log_queue.push(message);
            cv.notify_one();
        }

        void processEntries() {
            std::unique_lock<std::mutex> lock(queue_mutex, std::defer_lock);
            while (true) {
                lock.lock();
                cv.wait(lock, [this] {
                    return !log_queue.empty() || exit_flag.load();
                });
                while (!log_queue.empty()) {
                    sout << log_queue.front();
                    log_queue.pop();
                }
                if (exit_flag.load()) {
                    break;
                }
                lock.unlock();
            }
        }
    };

#pragma endregion

#pragma region TESSERACT_OPENMP          /* Tesseract Implementation for Thread Local Tesseracts'  */

    static std::atomic<int> TesseractThreadCount(0);

    struct TesseractOCR {
        tesseract::TessBaseAPI *ocrPtr {nullptr};

        TesseractOCR()                                         = default;
        TesseractOCR(const TesseractOCR &)                     = default;
        auto operator=(const TesseractOCR &) -> TesseractOCR & = default;
        TesseractOCR(TesseractOCR &&)                          = delete;
        auto operator=(TesseractOCR &&) -> TesseractOCR      & = delete;

        void init(const std::string &lang = "eng", ImgMode mode = ImgMode::document) {
            if (ocrPtr == nullptr) {
                ocrPtr = new tesseract::TessBaseAPI();

                serr << WARNING << "Created New Tesseract" << END << '\n';

                TesseractThreadCount.fetch_add(1, std::memory_order_relaxed);

                if (ocrPtr->Init(nullptr, lang.c_str()) != 0) {
                    serr << WARNING << "Deleting OCR Ptr" << END << '\n';
                    delete ocrPtr;
                    ocrPtr = nullptr;
                    throw std::runtime_error("Could not initialize "
                                             "tesseract.");
                }
                if (mode == ImgMode::image) { /* Optimized for Complex Images */
                    sout << "Image mode set\n";
                    ocrPtr->SetPageSegMode(tesseract::PSM_AUTO);
                }
            }
        }
        ~TesseractOCR() {
            serr << WARNING << "TesseractOCR Destructor Called on thread " << omp_get_thread_num()
                 << END << "\n";

            if (ocrPtr != nullptr) {
                serr << WARNING << "Destructor call ocrPtr was not Null" << END << '\n';
                ocrPtr->Clear();
                ocrPtr->End();
                delete ocrPtr;
                ocrPtr = nullptr;
            }
        }
        auto operator->() const -> tesseract::TessBaseAPI * { return ocrPtr; }
    };

    static TesseractOCR thread_local_tesserat;

#pragma omp threadprivate(thread_local_tesserat)

    // inline void cleanupOpenMPTesserat() {
    //     serr << "cleanupOpenMPTesserat Called on thread " << omp_get_thread_num() << END << '\n';
    //     if (thread_local_tesserat.ocrPtr != nullptr) {
    //         serr << "Cleanup: Thread Local not NullPtr, clearing" << END << '\n';
    //         thread_local_tesserat.ocrPtr->End();
    //         delete thread_local_tesserat.ocrPtr;
    //         thread_local_tesserat.ocrPtr = nullptr;
    //         TesseractThreadCount.fetch_sub(1, std::memory_order_relaxed);
    //     }
    // }

    inline void cleanupOpenMPTesserat() {
#pragma omp parallel
        {
            serr << WARNING << "cleanupOpenMPTesserat Called" << END << '\n';

            serr << WARNING << "Cleanup: Thread Local OCR pointer clearing on thread "
                 << omp_get_thread_num();

            if (thread_local_tesserat.ocrPtr != nullptr) {
                serr << WARNING << "Cleanup: Thread Local not NullPtr, clearing" << END << '\n';
                thread_local_tesserat.ocrPtr->End();
                delete thread_local_tesserat.ocrPtr;
                thread_local_tesserat.ocrPtr = nullptr;
                TesseractThreadCount.fetch_sub(1, std::memory_order_relaxed);

                // TesseractThreadCount.fetch_sub(1, std::memory_order_relaxed);

                /* Note: Manually calling Destructor is Bad Practice */
                // thread_local_tesserat.~TesseractOCR();

                // thread_local_tesserat.ocrPtr = nullptr;
            }
        }
    }

#pragma endregion

    /// ImgProcessor : Core Class for Image Processing and Text Extraction
    /// Provides an efficient, high performance implementation
    /// of Text Extraction from Images.
    /// Supports Parallelized Image Processing and maintains an in-memory cache.
    /// Uses an Atomic Unordered Map for Safe Wait-Free parallel access
    /// Cache retrieval logic is determined by the SHA256 hash of the Image bytes
    /// The SHA256 Byte Hash enables duplicate images to not be processed even if
    /// the file names or paths differ.

#pragma region imgstr_core

    struct WriteMetadata {
        std::string output_path;
        std::string write_timestamp;
        bool        output_written = false;
    };

    struct Image {
        std::string path;
        std::string text_content;
        std::string image_sha256;
        std::string time_processed;
        std::string content_fuzzhash;
        std::size_t text_size;
        std::size_t image_size;

        mutable WriteMetadata write_info;

        mutable std::unique_ptr<folly::SharedMutex> mutex;

        Image(std::string        img_hash,
              std::string        path,
              const std::string &text_content,
              size_t             image_size = 0)
            : mutex(nullptr),
              path(std::move(path)),
              image_size(image_size),
              text_size(text_content.size()),
              text_content(text_content),
              image_sha256(std::move(img_hash)),
              time_processed(getCurrentTimestamp()) {}

        void updateWriteInfo(const std::string &output_path,
                             const std::string &write_timestamp,
                             bool               output_written) const {
            if (!mutex) {
                mutex = std::make_unique<folly::SharedMutex>();
            }
            std::unique_lock<folly::SharedMutex> writerLock(*mutex);
            write_info.output_path     = output_path;
            write_info.write_timestamp = write_timestamp;
            write_info.output_written  = output_written;
        }

        auto readWriteInfoSafe() const -> WriteMetadata {
            if (!mutex) {
                return write_info;
            }
            std::shared_lock<folly::SharedMutex> readerLock(*mutex);
            return write_info;
        }

        auto getName() const -> std::string {
            auto lastSlash = path.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                return path.substr(lastSlash + 1);
            }
            return path;
        }
    };

    class ImgProcessor {
      private:
        ImgMode                                             img_mode;
        CORES                                               num_cores;
        std::string                                         dir;
        std::vector<std::string>                            queued;
        std::unordered_set<std::string>                     files;
        std::unordered_set<std::string>                     processed;
        folly::AtomicUnorderedInsertMap<std::string, Image> cache;
        std::atomic<double>                                 totalProcessingTime {0.0};
        std::atomic<int>                                    processedImagesCount {0};
        std::unique_ptr<AsyncLogger>                        logger;

        static constexpr char path_separator = '/';
#ifdef _WIN32
        static constexpr path_separator = '\\';
#endif

        /**
         * @brief Process an Image File if not already Processed as dictated by the
         Cache.
         * Reads File Bytes, checks Hash of Image, pulls from the Cache if it exists,
         or sends Bytes to Tesseract to process to Text.
         * @param file
         * @return std::optional<std::reference_wrapper<const Image>>

         */
        auto processImageFile(const std::string &file)
            -> std::optional<std::reference_wrapper<const Image>> {
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
                }
                std::string img_text = getTextOCR(data, "eng", img_mode);

                addProcessingTime(totalProcessingTime, getDuration(start));

                Image image(img_hash, file, img_text, data.size());

                const auto &cachedImage = cache.emplace(img_hash, std::move(image)).first->second;

                processed.insert(file);

                return std::cref(cachedImage);

            } catch (const std::exception &e) {
                printFileProcessingFailure(file, e.what());
                return std::nullopt;
            }
        }

        auto getImageOrProcess(const std::string &file_path, ISOLang lang = ISOLang::en)
            -> std::optional<std::reference_wrapper<const Image>> {
            return processImageFile(file_path);
        }

        auto getFromCacheIfExists(const std::string &img_sha)
            -> std::optional<std::reference_wrapper<const Image>> {
            auto img_from_cache = cache.find(img_sha);
            if (img_from_cache != cache.end()) {
                return std::cref(img_from_cache->second);
            }
            return std::nullopt;
        }

        auto processCurrentFiles() -> std::vector<std::string> {
            if (files.empty()) {
                logger->log() << "Files are empty";
                return {};
            }

            std::vector<std::string> processedText;

            for (const auto &file: files) {
                auto image = processImageFile(file);
                if (image) {
                    processedText.emplace_back(image.value().get().text_content);
                }
            }

            return processedText;
        }

        void ifValidImageFileAppendQueue(const std::string &path) {
            if (isImageFile(path)) {
                if (files.find(path) == files.end()) {
                    files.insert(path);
                    queued.emplace_back(path);
                }
            }
        }

        void addProcessingTime(std::atomic<double> &totalTime, double timeToAdd) {
            processedImagesCount.fetch_add(1, std::memory_order_seq_cst);

            double current = totalTime.load(std::memory_order_relaxed);
            double newTime = 0.0;
            do {
                newTime = current + timeToAdd;
            } while (!totalTime.compare_exchange_weak(
                current, newTime, std::memory_order_relaxed, std::memory_order_relaxed));
        }

        auto getAverageProcessingTime() -> double {
            return totalProcessingTime.load() / processedImagesCount.load();
        }

        void initLog() {
#ifdef _DEBUGAPP
            logger->log() << ERROR << "DEBUG FLAGS ON\n" << END;
#endif

            logger->log() << "Processor Initialized" << '\n'
                          << "Threads Available: " << BOLD_WHITE << omp_get_max_threads() << END
                          << "\nCores Available: " << BOLD_WHITE << omp_get_num_procs() << END
                          << "\nCores Active: " << BOLD_WHITE << omp_get_num_threads() << END
                          << '\n';
        }

        void printCacheHit(const std::string &file) {
            logger->log() << '\n'
                          << SUCCESS_TICK << GREEN << "  Cache Hit : " << END << file << '\n';
        }

        void printFileProcessingFailure(const std::string &file, const std::string &err_msg) {
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
                          << "File already exists - " << END << BOLD_WHITE << output_path << END
                          << "    Are you sure you want to overwrite the file?" << '\n';
        }

        void filesAlreadyProcessedLog() {
            logger->log() << BOLD_WHITE << "All files already processed." << END;
        }

        void printOutputAlreadyWritten(const Image &image) {
            logger->log() << DELIMITER_STAR << '\n'
                          << WARNING << image.getName() << " Already Processed and written to "
                          << END << image.write_info.output_path << " at "
                          << image.write_info.write_timestamp << '\n';
        }

        void printProcessingFile(const std::string &file) {
            logger->log() << BOLD_WHITE << "Processing " << END << BRIGHT_WHITE << file << END
                          << '\n';
        }

        void printProcessingDuration(double duration_ms) {
            logger->log() << DELIMITER_STAR << '\n'
                          << BOLD_WHITE << queued.size() << END
                          << " Files Processed and Converted in " << BRIGHT_WHITE << duration_ms
                          << " seconds\n"
                          << END << DELIMITER_STAR << "\n";
        }

        void printImagesInfo() {
            const int width = 20;

            auto logstream = logger->stream();

            logstream << DELIMITER_STAR << '\n'
                      << BOLD_WHITE << "textract Processing Results\n\n"
                      << files.size() << END << " images proccessed\n"
                      << DELIMITER_STAR << '\n';

            for (const auto &img_sha: cache) {
                const Image &img = img_sha.second;

                sout << llvm::formatv("{0}SHA256:          {1}{2}\n"
                                      "{3}Path:            {1}{4}\n"
                                      "{3}Image Size:      {1}{5:N} bytes\n"
                                      "{3}Text Size:       {1}{6:N} bytes\n"
                                      "{3}Processed Time:  {1}{7}\n"
                                      "{3}Output Path:     {1}{8}\n"
                                      "{3}Output Written:  {1}{9}\n"
                                      "{3}Write Timestamp: {1}{10}\n"
                                      "{11}\n",
                                      Ansi::GREEN_BOLD,
                                      Ansi::END,
                                      img.image_sha256,
                                      Ansi::BLUE,
                                      img.path,
                                      img.image_size,
                                      img.text_size,
                                      img.time_processed,
                                      img.write_info.output_path,
                                      (img.write_info.output_written ? "Yes" : "No"),
                                      img.write_info.write_timestamp,
                                      Ansi::DELIMITER_ITEM)
                            .str();

                logstream << GREEN_BOLD << std::left << std::setw(width) << "SHA256: " << END
                          << img.image_sha256 << '\n'
                          << BLUE << std::left << std::setw(width) << "Path: " << END << img.path
                          << '\n'
                          << BLUE << std::left << std::setw(width) << "Image Size: " << END
                          << img.image_size << " bytes\n"
                          << BLUE << std::left << std::setw(width) << "Text Size: " << END
                          << img.text_size << " bytes\n"
                          << BLUE << std::left << std::setw(width) << "Processed Time: " << END
                          << img.time_processed << '\n'
                          << BLUE << std::left << std::setw(width) << "Output Path: " << END
                          << img.write_info.output_path << '\n'
                          << BLUE << std::left << std::setw(width) << "Output Written: " << END
                          << (img.write_info.output_written ? "Yes" : "No") << '\n'
                          << BLUE << std::left << std::setw(width) << "Write Timestamp: " << END
                          << img.write_info.write_timestamp << '\n'
                          << DELIMITER_ITEM << '\n';
            }

            logstream.flush();
        }

        void destructionLog() {
            logger->log() << LIGHT_GREY << "Destructor called - freeing " << BRIGHT_WHITE
                          << imgstr::TesseractThreadCount.load(std::memory_order_relaxed) << END
                          << " Tesseracts\n"
                          << END << "\nAverage Image Processing Latency: " << BOLD_WHITE
                          << getAverageProcessingTime() << END << " ms\n"
                          << '\n'
                          <<

                BRIGHT_WHITE << "Total Images Processed :: " << BOLD_WHITE << processedImagesCount
                          << END << '\n';
        }

      public:
        template <typename T>
        ImgProcessor(size_t capacity = 1000, T cores = 1): ImgProcessor(capacity) {
            setCores(cores);
        }

        ImgProcessor(size_t capacity = 1000)

            : num_cores(CORES::single),
              logger(std::make_unique<AsyncLogger>()),
              cache(folly::AtomicUnorderedInsertMap<std::string, Image>(capacity)),
              img_mode(ImgMode::document) {
            initLog();
            setCores(num_cores); // explicitly set cores
        }

        ImgProcessor(const ImgProcessor &)                     = delete;
        ImgProcessor(ImgProcessor &&)                          = delete;
        auto operator=(const ImgProcessor &) -> ImgProcessor & = delete;
        auto operator=(ImgProcessor &&) -> ImgProcessor      & = delete;

        ~ImgProcessor() {
            destructionLog();

            cleanupOpenMPTesserat();
        }

        auto getImageText(const std::string &file_path,
                          ISOLang            lang = ISOLang::en) -> std::optional<std::string> {
            auto image = processImageFile(file_path);

            if (image) {
                return image.value().get().text_content;
            }

            return std::nullopt;
        }

        void processImagesDir(const std::string &directory,
                              bool               write_output = false,
                              const std::string &output_path  = "") {
            auto files = getFilePaths(directory);
            if (!files) {
                serr << llvm::formatv(
                    "Error extracting paths from {0}:{1}\n", directory, getErr(files.takeError()));
                return;
            }

            for (const auto &filePath: files.get()) {
                ifValidImageFileAppendQueue(filePath);
            }

            if (write_output) {
                convertImagesToTextFilesParallel(output_path);
            }
        }

        void simpleProcessDir(const std::string &directory, const std::string &output_path = "") {
            std::vector<std::string> imageFiles;

            auto files = getFilePaths(directory);

            if (!files) {
                serr << llvm::formatv(
                    "Error extracting paths from {0}:{1}\n", directory, getErr(files.takeError()));
                return;
            }
            for (const auto &file: files.get()) {
                if (isImageFile(file)) {
                    imageFiles.push_back(file);
                }
            }

#pragma omp parallel for
            for (const auto &imagePath: imageFiles) {
                START_TIMING();
                auto file_content = readBytesFromFile(imagePath);
                auto img_text     = getTextOCRNoClear(file_content);
                auto out_path     = createQualifiedFilePath(imagePath, output_path, ".txt");
                writeTextToFile(img_text, out_path.get());
                END_TIMING("simple: file processed and written ");
            }
        }

        void writeTextToFile(const std::string &content, const std::string &output_path) {
            if (fileExists(output_path)) {
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

        ///   @brief Converts an image file to a text file. If no Directory is passed,
        ///   the Text File is created in the same directory. Checks if the file is
        ///   already processed, if not uses tesseract to process the image and then
        ///   Write results to a Text File.
        ///   @param input_file The input image file.
        ///   @param output_dir The output directory (optional).
        ///   @param lang The language of the text (optional, default: en).
        ///   @return void
        ///   @usage convertImageToTextFile("image.jpg", "output_dir", ISOLang::en);
        ///
        void convertImageToTextFile(const std::string &input_file,
                                    const std::string &output_path = "",
                                    ISOLang            lang        = ISOLang::en) {
            if (output_path != "") {
                Unwrap<StdErr>(createDirectoryForFile(output_path));
            }

            auto output_file = createQualifiedFilePath(input_file, output_path, ".txt");

            if (!output_file) {
                serrfmt("Failed to Create Qualified Path:{0}\n", input_file);
                return;
            }

            auto imageOpt = getImageOrProcess(input_file, lang);

            if (!imageOpt) {
                serrfmt("Failed to Retrieve or Process Image : {0}\n", input_file);
                return;
            }

            const Image &image = imageOpt.value().get();

            if (image.write_info.output_written) {
                printOutputAlreadyWritten(image);
                return;
            }

            writeTextToFile(image.text_content, output_file.get());

            image.updateWriteInfo(output_file.get(), getCurrentTimestamp(), true);
        }

        void convertImagesToTextFilesParallel(const std::string &output_dir = "",
                                              ISOLang            lang       = ISOLang::en) {
            if (!output_dir.empty() && !fileExists(output_dir) && !createDirectories(output_dir)) {
                return;
            }

            if (queued.empty()) {
                filesAlreadyProcessedLog();
                return;
            }

#pragma omp parallel for
            for (const auto &file: queued) {
                START_TIMING();
                convertImageToTextFile(file, output_dir, lang);
                END_TIMING("parallel() - file processed ");
            }
            queued.clear();
        }

        void convertImagesToTextFiles(const std::string &output_dir = "",
                                      ISOLang            lang       = ISOLang::en) {
            if (output_dir != "") {
                Unwrap<StdErr>(createDirectories(output_dir));
            }

            if (queued.empty()) {
                filesAlreadyProcessedLog();
                return;
            }

#pragma omp parallel for
            for (const auto &file: queued) {
                convertImageToTextFile(file, output_dir, lang);
            }

            queued.clear();
        }

        void generatePDF(const std::string &input_path,
                         const std::string &output_path,
                         const char        *datapath = DATAPATH) {
            try {
                createPDF(input_path, output_path);

            } catch (const std::exception &e) {
                logger->log() << ERROR << "Failed to Generate PDF : " << e.what() << END;
            }
        }

        /* utils */

        void setImageMode(ImgMode img_mode) { this->img_mode = img_mode; }

        template <typename T>
        void setCores(T cores) {
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
                int numThreads = std::min(static_cast<int>(cores), omp_get_num_procs());
                omp_set_num_threads(numThreads);
            } else {
                static_assert(false, "Unsupported type for setCores");
            }
        }

        void resetCache(size_t new_capacity) {
            cache = folly::AtomicUnorderedInsertMap<std::string, Image>(new_capacity);
        }

        /// @brief Add Files to the Queue for Processing. Methods such as
        /// convertImagesToTextFiles(outputDir) will then begin processing all Files from the Queue
        ///
        /// @tparam Container
        /// @param fileList
        /// @code{.cpp}
        ///     imageTranslator.addFiles(fpaths)
        /// @endcode
        template <typename Container>
        void addFiles(const Container &fileList) {
            for (const auto &file: fileList) {
                auto [_, inserted] = files.emplace(file);
                if (inserted) {
                    queued.emplace_back(file);
                }
            }
        }

        template <typename... FileNames>
        auto processImages(FileNames... fileNames) -> std::vector<std::string> {
            addFiles({fileNames...});
            return processCurrentFiles();
        }

        void printFiles() {
            for (const auto &file: files) {
                logger->log() << file;
            }
        }

        void getResults() { printImagesInfo(); }
    };

#pragma endregion

    /* Implementations */

#pragma region TEXT_SIMILARITY_IMPL

    inline auto levenshteinScore(std::string a, std::string b) -> size_t {
        size_t m = a.size();
        size_t n = b.size();

        std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));

        for (size_t i = 1; i < m; i++) {
            dp[i][0] = i;
        }

        for (size_t i = 1; i < n; i++) {
            dp[0][i] = i;
        }

        for (size_t i = 1; i <= m; i++) {
            for (size_t j = 1; j <= n; j++) {
                size_t equal = a[i - 1] == b[j - 1] ? 0 : 1;

                dp[i][j] = std::min(
                    dp[i - 1][j - 1] + equal, std::min(dp[i - 1][j] + 1, dp[i][j - 1] + 1));
            }
        }

        return dp[m][n];
    }

#pragma endregion

#pragma region CRYPTOGRAPHY_IMPL           /*    OpenSSL4 Cryptography Implementations    */

    inline auto computeSHA256(const std::vector<unsigned char> &data) -> std::string {
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

        std::vector<unsigned char> hash(EVP_MD_size(EVP_sha256()));

        unsigned int lengthOfHash = 0;

        if (EVP_DigestFinal_ex(mdContext, hash.data(), &lengthOfHash) != 1) {
            EVP_MD_CTX_free(mdContext);
            throw std::runtime_error("Failed to finalize digest");
        }

        EVP_MD_CTX_free(mdContext);

        std::stringstream sha256;
        for (unsigned int i = 0; i < lengthOfHash; ++i) {
            sha256 << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return sha256.str();
    }

    inline auto computeSHA256(const std::string &filePath) -> std::string {
        std::ifstream file(filePath, std::ifstream::binary);
        if (!file) {
            throw std::runtime_error("Could not open file: " + filePath);
        }
        std::vector<unsigned char> data(std::istreambuf_iterator<char>(file), {});
        return computeSHA256(data);
    }

#pragma endregion

#pragma region OPENMP_UTILS

    inline void logThreadUse() {
#pragma omp parallel
        {
#pragma omp single
            { std::cout << "Actual threads in use: " << omp_get_num_threads() << std::endl; }
        }
    }

#pragma endregion

#pragma region TESSERACT_OPENMP

#pragma region OPENCV_IMPL                /* OpenCV Image Processing Implementations */

    static const std::unordered_set<std::string> validExtensions = {
        ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tif"};

    inline auto isImageFile(const llvm::StringRef &path) -> bool {
        return validExtensions.find(llvm::sys::path::extension(path).str()) !=
               validExtensions.end();
    }

    inline void tesseractInvokeLog(ImgMode img_mode) {
        serr << ERROR << "getTextOCR "
             << (img_mode == ImgMode::document ? "document mode " : "image mode ") << END
             << " -> called from thread: " << omp_get_thread_num() << '\n';
    }

    inline auto getTextOCR(const std::vector<unsigned char> &file_content,
                           const std::string                &lang,
                           ImgMode img_mode = ImgMode::document) -> std::string {
        /* Leptonica reads 40% or more faster than OpenCV */

#ifdef _DEBUGAPP
        tesseractInvokeLog(img_mode);
#endif

        if (thread_local_tesserat.ocrPtr == nullptr) {
            thread_local_tesserat.init(lang, img_mode);
        }
        Pix *image =
            pixReadMem(static_cast<const l_uint8 *>(file_content.data()), file_content.size());
        if (image == nullptr) {
            throw std::runtime_error("Failed to load image from memory "
                                     "buffer");
        }

        thread_local_tesserat->SetImage(image);

        char       *rawText = thread_local_tesserat->GetUTF8Text();
        std::string outText(rawText);

        delete[] rawText;

        pixDestroy(&image);
        thread_local_tesserat->Clear();
        return outText;
    };

    inline void createPDF(const std::string &input_path,
                          const std::string &output_path,
                          const char        *datapath) {
        bool textonly = false;

        auto *api = new tesseract::TessBaseAPI();
        if (api->Init(datapath, "eng") != 0) {
            fprintf(stderr, "Could not initialize tesseract.\n");
            delete api;
            return;
        }

        auto *renderer = new tesseract::TessPDFRenderer(output_path.c_str(), datapath, textonly);

        bool succeed = api->ProcessPages(input_path.c_str(), nullptr, 0, renderer);
        if (!succeed) {
            fprintf(stderr, "Error during processing.\n");
        }

        api->End();
        delete api;
        delete renderer;
    }

    inline auto getTextOCRNoClear(const std::vector<unsigned char> &file_content,
                                  const std::string                &lang,
                                  ImgMode                           img_mode) -> std::string {
        if (thread_local_tesserat.ocrPtr == nullptr) {
            thread_local_tesserat.init(lang, img_mode);
        }
        Pix *image =
            pixReadMem(static_cast<const l_uint8 *>(file_content.data()), file_content.size());
        if (image == nullptr) {
            throw std::runtime_error("Failed to load image from memory "
                                     "buffer");
        }

        thread_local_tesserat->SetImage(image);

        char       *rawText = thread_local_tesserat->GetUTF8Text();
        std::string outText(rawText);

        delete[] rawText;
        pixDestroy(&image);

        return outText;
    };

    inline auto
        getTextImgFile(const std::string &file_path, const std::string &lang) -> std::string {
        if (thread_local_tesserat.ocrPtr == nullptr) {
            thread_local_tesserat.init(lang);
        }

        Pix *image = pixRead(file_path.c_str());

        thread_local_tesserat->SetImage(image);

        char       *rawText = thread_local_tesserat->GetUTF8Text();
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
        getTextOCROpenCV(const std::vector<uchar> &file_content, const std::string &lang) {
        if (!thread_local_tesserat.ocrPtr) {
            sout << "New Tesserat instance created\n" << std::endl;
            thread_local_tesserat.init(lang);
        }

        cv::Mat img = cv::imdecode(file_content, cv::IMREAD_COLOR);
        if (img.empty()) {
            throw std::runtime_error("Failed to load image from buffer");
        }

        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        thread_local_tesserat->SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);
        std::string outText(thread_local_tesserat->GetUTF8Text());

        return outText;
    };

    inline std::string extractTextFromImageBytes(const std::vector<uchar> &file_content,
                                                 const std::string        &lang = "eng") {
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
        extractTextFromImageFile(const std::string &file_path, const std::string &lang) {
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

    inline std::string extractTextFromImage(const std::string &file_path, ISOLang lang) {
        std::string langCode = isoToTesseractLang(lang);
        return extractTextFromImageFile(file_path, langCode);
    }
#endif

#pragma endregion

//
#pragma region FILE_IO_IMPL               /* STL File I/O Implementations */

    inline auto readBytesFromFile(const std::string &filename) -> std::vector<unsigned char> {
#ifdef _DEBUGFILEIO
        sout << "Converting to char* " << filename << std::endl;
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

    inline void writeToNewFile(const std::string &content, const std::string &output_path) {
        if (fileExists(output_path)) {
            serr << "Error: File already exists - " << output_path << '\n';
            return;
        }
        std::ofstream outFile(output_path);
        if (!outFile) {
            serr << "Error opening file: " << output_path << '\n';
            return;
        }
        outFile << content;
    }

    inline auto getCurrentTimestamp() -> std::string {
        auto now       = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream sst;
        sst << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return sst.str();
    }

#pragma endregion

#pragma region SYSTEM_IMPL                /* System Environment Util Implementations */

    auto inline fileExists(const std::string &path) -> bool { return llvm::sys::fs::exists(path); }

    using high_res_clock = std::chrono::high_resolution_clock;
    using time_point     = std::chrono::time_point<high_res_clock>;

    inline auto getStartTime() -> time_point { return high_res_clock::now(); }

    inline auto getDuration(const time_point &startTime) -> double {
        auto endTime = high_res_clock::now();
        return std::chrono::duration<double>(endTime - startTime).count();
    }

    inline void printDuration(const time_point &startTime, const std::string &msg) {
        auto endTime  = high_res_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        llvm::formatv("{0}{1}{2}{3}{4}{5}\n", BOLD, CYAN, msg, duration.count(), END);
    }

    inline void printSystemInfo() {
#ifdef __clang__
        sout << "Clang version: " << __clang_version__;
#else
        sout << "Not using Clang." << std::endl;
#endif

#ifdef _OPENMP
        sout << "openmp is enabled.\n";
#else
        sout << "OpenMP is not enabled." << std::endl;
#endif
    };

#pragma endregion

} // namespace imgstr

#endif // TEXTRACT_H
