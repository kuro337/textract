
#include <iostream>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>

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
