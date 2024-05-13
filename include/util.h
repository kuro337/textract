
// util.h
#ifndef UTIL_H
#define UTIL_H

#include "llvm/Support/Regex.h"
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
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
    } else if constexpr (std::is_same_v<Tag, StdErr>) {
        if (!expected) {
            llvm::Error err = expected.takeError();
            llvm::errs() << "Error: " << llvm::toString(std::move(err)) << '\n';
            return false;
        }
    } else if constexpr (std::is_same_v<Tag, NoThrow>) {
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

/* Valid Image File Extensions */
static const std::unordered_set<std::string_view> validExtensions = {
    "jpg", "jpeg", "png", "bmp", "gif", "tif"};

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

#endif // UTIL_H