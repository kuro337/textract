// fs.h
#ifndef FS_H
#define FS_H

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

auto getFilePaths(const llvm::Twine &directoryPath) -> llvm::Expected<std::vector<std::string>>;

auto getFileInfo(const llvm::Twine &Path) -> llvm::Expected<llvm::sys::fs::file_status>;

auto createDirectories(const llvm::StringRef &path) -> llvm::Expected<bool>;

auto createDirectoryForFile(const llvm::Twine &filePath) -> llvm::Expected<bool>;

auto createQualifiedFilePath(const llvm::StringRef &fileName,
                             const llvm::StringRef &directory,
                             const llvm::StringRef &extension,
                             char osSeparator = '/') -> llvm::Expected<std::string>;

auto deleteFile(const llvm::Twine &filePath) -> llvm::Error;

auto deleteDirectory(const llvm::Twine &directoryPath) -> llvm::Error;

/**
 * @brief Deletes multiple directories. This function takes a variable number of paths and attempts
 * to delete each one. It aggregates any errors encountered into a single error object that is then
 * returned.
 *
 * @tparam Paths Variadic template to accept multiple path arguments of potentially differing types.
 * @param paths One or more directory paths to delete.
 * @return llvm::Error Aggregated error object containing any errors that occurred during the
 * deletion process. A successful deletion results in a success error state (empty error).
 *
 * @code{.cpp}
 *
 *      if (auto err = deleteDirectories(tempDirectory, tempBasePath, tempBaseResolvedPath)) {
 *      llvm::errs() << "Error deleting directories: " << llvm::toString(std::move(err)) << '\n';
 *      }
 *
 * @endcode
 */
template <typename... Paths>
auto deleteDirectories(const Paths &...paths) -> llvm::Error {
    llvm::Error aggregatedError = llvm::Error::success();
    (..., (aggregatedError = llvm::joinErrors(std::move(aggregatedError), deleteDirectory(paths))));
    return aggregatedError;
}

auto getErr(llvm::Error err) -> std::string;

#endif // FS_H
