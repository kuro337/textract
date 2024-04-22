
// util.h
#ifndef UTIL_H
#define UTIL_H

#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_set>

struct Throw {};
struct NoThrow {};
struct StdErr {};

template <typename... Args>
inline void serrfmt(const char *message, Args &&...args) {
    llvm::errs() << llvm::formatv(message, std::forward<Args>(args)...) << "\n";
}
template <typename... Args>
inline void soutfmt(const char *message, Args &&...args) {
    llvm::outs() << llvm::formatv(message, std::forward<Args>(args)...) << "\n";
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

static const std::unordered_set<std::string_view> validExtensions = {
    "jpg", "jpeg", "png", "bmp", "gif", "tif"};

inline auto isImageFile(const llvm::StringRef &path) -> bool {
    if (auto pos = path.find_last_of('.'); pos != llvm::StringRef::npos) {
        return validExtensions.contains(path.drop_front(pos + 1).lower());
    }
    return false;
}

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

#endif // UTIL_H