// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <condition_variable>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/raw_ostream.h>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

static inline auto &sout = llvm::outs();
static inline auto &serr = llvm::errs();

#pragma region SYSTEM_UTILS               /* System Environment helpers */

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

TimePoint getStartTime();

double getDuration(const TimePoint &start);

void printDuration(const TimePoint &startTime, const std::string &msg = "");

void printSystemInfo();

#pragma endregion

/// @brief Non Blocking Asynchronous Logger with ANSI Escaping
/// Supports Logging Immediately or building a Stream and Flushing Asynchronously
///
/// @code{.cpp}
///     std::unique_ptr<AsyncLogger> logger;
///     logger->log() << "Files are empty";
///     auto logstream = logger->stream();
///     logstream << "streaming text" ...
///     logstream.flush()
/// @endcode
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

    template <typename... Args>
    void logFormatted(const std::string &format, Args &&...args) {
        log() << fmtstr(format.c_str(), std::forward<Args>(args)...);
    }

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

#endif // LOGGER_H
