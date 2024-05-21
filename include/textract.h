#ifndef TEXTRACT_H
#define TEXTRACT_H

#include <constants.h>
#include <conversion.h>
#include <crypto.h>
#include <folly/AtomicUnorderedMap.h>
#include <folly/SharedMutex.h>
#include <fs.h>
#include <future>
#include <ktesseract.h>
#include <logger.h>
#include <omp.h>
#include <util.h>

namespace imgstr {

#pragma region TEXT_SIMILARITY            /* Text Similarity Declarations */

    size_t levenshteinScore(std::string a, std::string b);

#pragma endregion

#pragma region FILE_IO                    /* File IO Declarations */

#ifdef _WIN32
    static constexpr char SEPARATOR = '\\';
#else
    static constexpr char SEPARATOR = '/';
#endif

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

#ifdef _USE_OPENCV

    std::string
        getTextOCROpenCV(const std::vector<unsigned char> &file_content, const std::string &lang);

    std::string extractTextFromImageBytes(const std::vector<unsigned char> &file_content,
                                          const std::string                &lang);

    std::string extractTextFromImageFile(const std::string &file_path, const std::string &lang);

    std::string extractTextFromImageFile(const std::string &file_path, ISOLang lang);
#endif

#pragma endregion

    /*
     * ImgProcessor : Core Class for Image Processing and Text Extraction
     * Provides an efficient, high performance implementation
     * of Text Extraction from Images.
     * Supports Parallelized Image Processing and maintains an in-memory cache.
     * Uses an Atomic Unordered Map for Safe Wait-Free parallel access
     * Cache retrieval logic is determined by the SHA256 hash of the Image bytes
     * The SHA256 Byte Hash enables duplicate images to not be processed even if
     * the file names or paths differ.
     */

#pragma region imgstr_core

    using namespace Ansi;
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

                             bool output_written) const {
            if (!mutex) {
                mutex = std::make_unique<folly::SharedMutex>();
            }
            std::unique_lock<folly::SharedMutex> writerLock(*mutex);
            write_info.output_path     = output_path;
            write_info.write_timestamp = write_timestamp;
            write_info.output_written  = output_written;
        }

        WriteMetadata readWriteInfoSafe() const {
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

        std::vector<std::string> processCurrentFiles() {
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

        double getAverageProcessingTime() {
            return totalProcessingTime.load() / processedImagesCount.load();
        }

        void initLog() {
#ifdef _DEBUGAPP
            logger->log() << fmtstr("{0}DEBUG FLAGS ON\n{1}", ERROR, END);
#endif

            logger->log() << fmtstr("Processor Initialized\nThreads Available: {0}{1}\nCores "
                                    "Available: {2}{3}\nCores Active: {4}{5}\n",
                                    BOLD_WHITE,
                                    omp_get_max_threads(),
                                    END,
                                    BOLD_WHITE,
                                    omp_get_num_procs(),
                                    END,
                                    BOLD_WHITE,
                                    omp_get_num_threads(),
                                    END);
        }

        void printCacheHit(const std::string &file) {
            logger->log() << fmtstr(
                "\n{0}{1}  Cache Hit : {2}{3}\n", SUCCESS_TICK, GREEN, END, file);
        }

        void printFileProcessingFailure(const std::string &file, const std::string &err_msg) {
            logger->log() << fmtstr(
                "Failed to Extract Text from Image file: {0}. Error: {1}\n", file, err_msg);
        }

        void printInputFileAlreadyProcessed(const std::string &file) {
            logger->log() << fmtstr("{0}\n{1}File at path : {2}{3}has already been "
                                    "processed to text\n",
                                    DELIMITER_STAR,
                                    WARNING,
                                    END,
                                    file);
        }

        void fileOpenErrorLog(const std::string &output_path) {
            logger->log() << fmtstr("{0}Error opening file: {1}", ERROR, output_path);
        }

        void overWriteLog(const std::string &output_path) {
            logger->log() << fmtstr("{0}WARNING:  {1}{2}File already exists - {3}{4}{5}    "
                                    "Are you sure you want to overwrite the file?\n",
                                    WARNING_BOLD,
                                    END,
                                    WARNING,
                                    END,
                                    BOLD_WHITE,
                                    output_path,
                                    END);
        }

        void filesAlreadyProcessedLog() {
            logger->log() << fmtstr("{0}All files already processed.{1}", BOLD_WHITE, END);
        }

        void printOutputAlreadyWritten(const Image &image) {
            logger->log() << fmtstr("{0}\n{1}{2} Already Processed and written to {3}{4} at {5}\n",
                                    DELIMITER_STAR,
                                    WARNING,
                                    image.getName(),
                                    END,
                                    image.write_info.output_path,
                                    image.write_info.write_timestamp);
        }

        void printProcessingFile(const std::string &file) {
            logger->log() << fmtstr(
                "{0}Processing {1}{2}{3}\n", BOLD_WHITE, END, BRIGHT_WHITE, file, END);
        }

        void printProcessingDuration(double duration_ms) {
            logger->log() << fmtstr("{0}\n{1}{2} Files Processed and Converted in {3}{4} "
                                    "seconds\n{5}{6}\n",
                                    DELIMITER_STAR,
                                    BOLD_WHITE,
                                    queued.size(),
                                    END,
                                    BRIGHT_WHITE,
                                    duration_ms,
                                    END,
                                    DELIMITER_STAR);
        }

        void printImagesInfo() {
            const int width = 20;

            auto logstream = logger->stream();

            logstream << fmtstr("{0}\n{1}textract Processing Results\n\n{2}{3} images "
                                "processed\n{4}\n",
                                DELIMITER_STAR,
                                BOLD_WHITE,
                                files.size(),
                                END,
                                DELIMITER_STAR);

            for (const auto &img_sha: cache) {
                const Image &img = img_sha.second;

                logstream << fmtstr("{0}SHA256:          {1}{2}\n"
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
                                    Ansi::DELIMITER_ITEM);
            }

            logstream.flush();
        }

        void destructionLog() {
            logger->log() << fmtstr("{0}Destructor called - freeing {1} {2} Tesseracts.\nAverage "
                                    "Image Processing Latency: {3} {4} ms.\n\n{5}Total Images "
                                    "Processed :: {6} {7}\n",
                                    LIGHT_GREY,
                                    BRIGHT_WHITE,
                                    TesseractThreadCount.load(std::memory_order_relaxed),
                                    END,
                                    BOLD_WHITE,
                                    getAverageProcessingTime(),
                                    END,
                                    BRIGHT_WHITE,
                                    BOLD_WHITE,
                                    processedImagesCount,
                                    END);
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

            setCores(num_cores);
        }

        ImgProcessor(const ImgProcessor &)                     = delete;
        ImgProcessor(ImgProcessor &&)                          = delete;
        auto operator=(const ImgProcessor &) -> ImgProcessor & = delete;
        auto operator=(ImgProcessor &&) -> ImgProcessor      & = delete;

        ~ImgProcessor() {
            destructionLog();
            completeAllThreads();
            cleanupOpenMPTesserat();
        }

        void completeAllThreads() {
#pragma omp barrier
            serr << WARNING << " All Threads Completed\n";
        }

        /// @brief Get Text from a Single Image File - part of the processing pipeline
        /// @param file_path
        /// @param lang = "en"
        /// @return std::optional<std::string>
        auto getImageText(const std::string &file_path,
                          ISOLang            lang = ISOLang::en) -> std::optional<std::string> {
            auto image = processImageFile(file_path);

            if (image) {
                return image.value().get().text_content;
            }

            return std::nullopt;
        }

        /// @brief Get Text from a Single Image File - for individual Static Calls
        /// @param file_path
        /// @param lang = "en"
        /// @return std::optional<std::string>
        auto getTextFromImage(const std::string &imagePath,
                              ISOLang            lang = ISOLang::en) -> std::string {
            auto file_content = readBytesFromFile(imagePath);
            auto img_text     = getTextOCRNoClear(file_content);

            return img_text;
        }

        /// @brief Convert a Single Image File and Write to an Output File
        /// @param file_path
        /// @param lang = "en"
        /// @return std::optional<std::string>
        llvm::Error processSingleImage(const std::string &imagePath,
                                       const std::string &outputPath,
                                       ISOLang            lang = ISOLang::en) {
            auto futureText = std::async(std::launch::async, [this, &imagePath, &lang] {
                return getTextFromImage(imagePath, lang);
            });

            auto out_path = createQualifiedFilePath(imagePath, outputPath, ".txt");
            auto img_text = futureText.get();

            // HandleError<Throw>(writeStringToFile(img_text, out_path.get()));

            return writeStringToFile(out_path.get(), img_text);
        }

        /// @brief Process Images Dynamically on a FIFO Basis - Optimized for Bulk Processing
        /// Workloads with Potential Duplicates
        /// @param directory
        /// @param write_output
        /// @param output_path
        void processImagesDir(const std::string &directory,
                              bool               write_output = false,
                              const std::string &output_path  = "") {
            sout << "Processing Images Dir\n";

            auto files = getFilePaths(directory);
            if (!files) {
                serrfmt(
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

        /// @brief Process Images from a Directory in One Batch - ideal for Independent Processing
        /// as an Isolated Job
        /// @param directory
        /// @param output_path
        void simpleProcessDir(const std::string &directory, const std::string &output_path = "") {
            std::vector<std::string> imageFiles;

            llvm::outs() << "Processing Images Dir" << '\n';

            auto files = getFilePaths(directory);

            if (!output_path.empty()) {
                try {
                    Unwrap<Throw>(createDirectories(output_path));
                } catch (const std::exception &e) {
                    serrfmt("Failed to Create Directory {0} : , not proceeding. ERR {1}\n",
                            output_path,
                            e.what());
                    return;
                }
            }

            if (!files) {
                serrfmt(
                    "Error extracting paths from {0}:{1}\n", directory, getErr(files.takeError()));
                return;
            }
            for (const auto &file: files.get()) {
                if (isImageFile(file)) {
                    imageFiles.push_back(file);
                }
            }

            llvm::outs() << "Processing Images within DIR, # images : " << imageFiles.size()
                         << '\n';

#pragma omp parallel for

            for (const auto &imagePath: imageFiles) {
                START_TIMING();
                auto file_content = readBytesFromFile(imagePath);
                auto img_text     = getTextOCRNoClear(file_content);
                auto out_path     = createQualifiedFilePath(imagePath, output_path, ".txt");

                HandleError<StdErr>(writeStringToFile(out_path.get(), img_text));

                END_TIMING("simple: file processed and written ");
            }
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
                                    bool               create_dir  = true,
                                    ISOLang            lang        = ISOLang::en) {
            if (create_dir && !output_path.empty()) {
                Unwrap<StdErr>(createDirectories(output_path));
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

            if (HandleError<StdErr>(writeStringToFile(output_file.get(), image.text_content))) {
                image.updateWriteInfo(output_file.get(), getCurrentTimestamp(), true);
            }
        }

        /// @brief Process Files with Available Cores Defined during Class Instantiation
        /// @param output_dir
        /// @param lang
        void convertImagesToTextFilesParallel(const std::string &output_dir = "",
                                              ISOLang            lang       = ISOLang::en) {
            if (!output_dir.empty() && !file_exists(output_dir) && !createDirectories(output_dir)) {
                return;
            }

            if (queued.empty()) {
                filesAlreadyProcessedLog();
                return;
            }

#pragma omp parallel for
            for (const auto &file: queued) {
                START_TIMING();
                convertImageToTextFile(file, output_dir, false, lang);
                END_TIMING("parallel() - file processed ");
            }
            queued.clear();
        }

        void convertImagesToTextFiles(const std::string &output_dir = "",
                                      ISOLang            lang       = ISOLang::en) {
            if (!output_dir.empty()) {
                Unwrap<StdErr>(createDirectories(output_dir));
            }

            if (queued.empty()) {
                filesAlreadyProcessedLog();
                return;
            }

#pragma omp parallel for
            for (const auto &file: queued) {
                convertImageToTextFile(file, output_dir, false, lang);
            }

            queued.clear();
        }

        void generatePDF(const std::string &input_path, const std::string &output_path) {
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
                        llvm::outs() << "CORES Enum : num Threads set " << 1 << '\n';
                        num_cores = CORES::single;

                        break;
                    case CORES::half:
                        omp_set_num_threads(omp_get_num_procs() / 2);
                        num_cores = CORES::half;

                        llvm::outs()
                            << "CORES Enum : num Threads set " << omp_get_num_procs() / 2 << '\n';
                        break;
                    case CORES::max:
                        omp_set_num_threads(omp_get_num_procs());
                        num_cores = CORES::max;

                        llvm::outs()
                            << "CORES Enum : num Threads set " << omp_get_num_procs() << '\n';
                        break;
                }
            } else if constexpr (std::is_integral<T>::value) {
                int numThreads = std::min(static_cast<int>(cores), omp_get_num_procs());

                llvm::outs() << "Integral Setting num Threads OpenMP" << numThreads << '\n';

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

#pragma region OPENMP_UTILS

    inline void logThreadUse() {
#pragma omp parallel
        {
#pragma omp single
            { soutfmt("Actual threads in use: {0:N}", omp_get_num_threads()); }
        }
    }

#pragma endregion

#pragma region OPENCV_IMPL                /* OpenCV Image Processing Implementations */

    inline void tesseractInvokeLog(ImgMode img_mode) {
        serrfmt("{3}getTextOCR {1}{4} -> called from thread {2}\n",
                (img_mode == ImgMode::document ? "document mode " : "image mode "),
                omp_get_thread_num(),
                ERROR,
                END);
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

} // namespace imgstr

#endif // TEXTRACT_H
