
// util.h
#ifndef UTIL_H
#define UTIL_H

#include "constants.h"
#include "logger.h"
#include <chrono>
#include <ctime>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Chrono.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Regex.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_set>

#ifdef DEBUG
#define DEBUG_LOG true
#else
#define DEBUG_LOG false
#endif

#ifdef DEBUG_THREADLOCAL
#define THREADLOCAL_DEBUG true
#else
#define THREADLOCAL_DEBUG false
#endif

#ifdef DEBUG_CACHE
#define CACHE_DEBUG true
#else
#define CACHE_DEBUG false
#endif

#ifdef DEBUG_MUTEX
#define MUTEX_DEBUG true
#else
#define MUTEX_DEBUG false
#endif

struct Throw {};
struct NoThrow {};
struct StdErr {};
namespace logging {
    enum class LogLevel { Info, Err };
}

struct DebugFlag {
    static constexpr bool enabled = DEBUG_LOG;
    static constexpr auto getPrefix() -> const char * { return ""; }
};

struct ThreadLocalConfig: DebugFlag {
    static constexpr bool enabled = THREADLOCAL_DEBUG;
    static constexpr auto getPrefix() -> const char * { return "[ThreadLocal] "; }
};

struct CacheConfig: DebugFlag {
    static constexpr bool enabled = CACHE_DEBUG;
    static constexpr auto getPrefix() -> const char * { return "[Cache] "; }
};

struct MutexConfig: DebugFlag {
    static constexpr bool enabled = MUTEX_DEBUG;
    static constexpr auto getPrefix() -> const char * { return "[Mutex] "; }
};

template <typename... Args>
inline void serrfmt(const char *message, Args &&...args) {
    llvm::errs() << llvm::formatv(message, std::forward<Args>(args)...) << "\n";
}

template <typename... Args>
inline void soutfmt(const char *message, Args &&...args) {
    llvm::outs() << llvm::formatv(message, std::forward<Args>(args)...) << "\n";
}

template <typename... Args>
inline auto fmtstr(const char *message, Args &&...args) -> std::string {
    return llvm::formatv(message, std::forward<Args>(args)...).str();
}

/// @brief Prints K/V's aligned
/// @tparam Args
/// @param args
/// @code{.cpp}
///   printKeyValuePairs(
///     std::make_pair("Total Elements", stats.elementsCount),
///     std::make_pair("Average Elements", stats.avgBucketSize),
///     std::make_pair("StdDev", llvm::formatv("{0:f4}", stats.stddev).str()),
///     std::make_pair("Minimum Bucket Count", stats.minBucketSize),
///     std::make_pair("Maximum Bucket Count", stats.maxBucketSize),
///     std::make_pair("Load Factor", llvm::formatv("{0:f4}", 100.42).str()));
/// @endcode
template <typename... Args>
inline void printKeyValuePairs(Args &&...args) {
    static_assert(sizeof...(args) % 2 == 0, "Arguments should be in key/value pairs");

    llvm::SmallVector<llvm::StringRef, 10> keys;
    llvm::SmallVector<std::string, 10>     values;

    auto processArgs = [&](const auto &key, const auto &value) {
        keys.push_back(key);
        values.push_back(llvm::formatv("{0}", value).str());
    };

    auto processAllArgs = [&](auto &&...kvPairs) {
        (processArgs(kvPairs.first, kvPairs.second), ...);
    };

    processAllArgs(std::forward<Args>(args)...);

    size_t maxKeyLength = 0;
    for (const auto &key: keys) {
        if (key.size() > maxKeyLength) {
            maxKeyLength = key.size();
        }
    }

    llvm::SmallString<32> formatString;
    llvm::raw_svector_ostream(formatString) << "{0,-" << maxKeyLength << "} : {1}\n";

    for (size_t i = 0; i < keys.size(); ++i) {
        llvm::outs() << llvm::formatv(formatString.c_str(), keys[i], values[i]);
    }
}

/// @brief Prints K/V's aligned
/// @tparam Args
/// @param kvPairs
/// @code{.cpp}
/// void testKV(const DistributionStats &stats) {
///   printKeyValuePairs(
///       {{"Total Elements", std::to_string(stats.elementsCount)},
///        {"Average Elements",
///         llvm::formatv("{0:f2}", stats.avgBucketSize).str()},
///        {"Standard Deviation", llvm::formatv("{0:f4}", stats.stddev).str()},
///        {"Minimum Bucket Count", std::to_string(stats.minBucketSize)},
///        {"Maximum Bucket Count", std::to_string(stats.maxBucketSize)},
///        {"Load Factor", llvm::formatv("{0:f4}", stats.loadFactor).str()}});
/// }
/// @endcode
inline void
    printKeyValuePairsList(std::initializer_list<std::pair<llvm::StringRef, std::string>> kvPairs) {
    llvm::SmallVector<llvm::StringRef, 10> keys;
    llvm::SmallVector<std::string, 10>     values;

    for (const auto &kv: kvPairs) {
        keys.push_back(kv.first);
        values.push_back(kv.second);
    }

    size_t maxKeyLength = 0;
    for (const auto &key: keys) {
        if (key.size() > maxKeyLength) {
            maxKeyLength = key.size();
        }
    }
    llvm::SmallString<32> formatString;
    llvm::raw_svector_ostream(formatString) << "{0,-" << maxKeyLength << "} : {1}\n";

    for (size_t i = 0; i < keys.size(); ++i) {
        llvm::outs() << llvm::formatv(formatString.c_str(), keys[i], values[i]);
    }
}

template <logging::LogLevel level, typename Config = DebugFlag, typename... Args>
constexpr void Debug(const char *message, Args &&...args) {
#ifdef _DEBUGAPP
    if constexpr (Config::enabled) {
        const char *prefix = Config::getPrefix();
        if constexpr (level == logging::LogLevel::Info) {
            llvm::outs() << prefix << llvm::formatv(message, std::forward<Args>(args)...) << "\n";
        } else if constexpr (level == logging::LogLevel::Err) {
            llvm::errs() << prefix << llvm::formatv(message, std::forward<Args>(args)...) << "\n";
        }
    }
#endif
}

/// @brief Utility to Handle llvm::Expected<T> when we care about if the Method Error Status alone
/// - StdErr  : Log Err to stderr
/// - Throw   : Throw an Exception if an Error exists
/// - NoThrow : Do nothing - only return true/false if an Error Exists or Not
/// @tparam Tag
/// @tparam T
/// @param expected
/// @return true
/// @return false
/// @code{.cpp}
///
///     try {
///       bool success = Unwrap<Throw>(result);
///     } catch () {}
///
///     bool log_stderr = Unwrap<StdErr>(result);
///     bool do_nothing = Unwrap<NoThrow>(result);
///
/// @endcode
/// @note Takes Ownership of the value , expects an Rvalue
template <typename Tag, typename T>
auto Unwrap(llvm::Expected<T> &&expected) -> bool {
    if constexpr (std::is_same_v<Tag, Throw>) {
        if (!expected) {
            llvm::Error err = expected.takeError();
            throw std::runtime_error(llvm::toString(std::move(err)));
        }
    }
    if constexpr (std::is_same_v<Tag, StdErr>) {
        if (!expected) {
            llvm::Error err = expected.takeError();
            llvm::errs() << "Error: " << llvm::toString(std::move(err)) << '\n';
            return false;
        }
    }
    if constexpr (std::is_same_v<Tag, NoThrow>) {
        if (!expected) {
            return false;
        }
    }

    return true;
}

/// @brief Template Utility to Handle llvm::Error Results
/// @tparam Tag
/// @param error
/// @return true
/// @return false
/// @code{.cpp}
///  HandleError<StdErr>(deleteFile(filePath);
///  HandleError<NoThrow>(processFiles(filePath);
/// @endcode
template <typename Tag>
inline auto HandleError(llvm::Error &&error) -> bool {
    if (error) {
        const std::string &errorMsg = llvm::toString(std::move(error));
        if constexpr (std::is_same_v<Tag, StdErr>) {
            llvm::errs() << "Error: " << errorMsg << '\n';
            return false;
        } else if constexpr (std::is_same_v<Tag, NoThrow>) {
            return false;
        }
    }
    return true;
}

/// @brief Validate if the File Path ends in a Valid Image Extension
/// @param path
/// @return true
/// @return false
inline auto isImageFile(const llvm::StringRef &path) -> bool {
    if (auto pos = path.find_last_of('.'); pos != llvm::StringRef::npos) {
        return validExtensions.contains(path.drop_front(pos + 1).lower());
    }
    return false;
}

/// @brief LevenshteinScore Algo to efficiently calculate the Distance between two 2D Entities
/// @param a
/// @param b
/// @return size_t
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

            dp[i][j] =
                std::min(dp[i - 1][j - 1] + equal, std::min(dp[i - 1][j] + 1, dp[i][j - 1] + 1));
        }
    }

    return dp[m][n];
}

/// @brief Check a String for Special Char Matches
/// @param str
/// @return true
/// @return false
inline auto hasSpecialChars(const llvm::StringRef &str) -> bool {
    llvm::Regex specialCharsRegex("[ \t\n\r\f\v]");
    return specialCharsRegex.match(str);
}

inline auto getCurrentTimestamp() -> std::string {
    auto                   now       = std::chrono::system_clock::now();
    llvm::sys::TimePoint<> timePoint = now;

    llvm::SmallString<32>     timestamp;
    llvm::raw_svector_ostream sst(timestamp);

    sst << llvm::formatv("{0:%Y-%m-%d %H:%M:%S.%N}", timePoint);

    return timestamp.str().str();
}

//     inline auto getCurrentTimestamp() -> std::string {
//         auto now       = std::chrono::system_clock::now();
//         auto in_time_t = std::chrono::system_clock::to_time_t(now);

//         std::stringstream sst;
//         sst << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
//         return sst.str();
//     }

#pragma region SYSTEM_IMPL                /* System Environment Util Implementations */

using high_res_clock = std::chrono::high_resolution_clock;
using time_point     = std::chrono::time_point<high_res_clock>;
using high_res_clock = std::chrono::high_resolution_clock;
using time_point     = std::chrono::time_point<high_res_clock>;
using duration       = std::chrono::duration<double>;

/// @brief Initialize the Start Time
/// @return time_point
inline auto getStartTime() -> time_point { return high_res_clock::now(); }

/// @brief Get the Time Elapsed from the Start Time in milliseconds, with
/// fractional part
/// @param startTime
/// @return double - Time in milliseconds with fractions
/// @code{.cpp}
///  auto start = getStartTime();
///  double duration = getDuration(start);
/// @endcode
inline auto getDuration(const time_point &startTime) -> double {
    auto endTime = high_res_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    return elapsed.count() / 1000.0;
}

inline void printDuration(const time_point &startTime, const std::string &msg) {
    auto endTime  = high_res_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    llvm::formatv("{0}{1}{2}{3}{4}{5}\n", Ansi::BOLD, Ansi::CYAN, msg, duration.count(), Ansi::END);
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

#endif // UTIL_H
