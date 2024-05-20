// fs.h
#ifndef FS_H
#define FS_H

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

///  @brief Check if the Path exists
///  @param path
///  @return bool
auto file_exists(const llvm::Twine &path) -> bool;

///  @brief Check if the Path is a Valid Directory
///  @param directoryPath
///  @return bool
auto is_dir(const llvm::Twine &directoryPath) -> bool;

/// @brief Write a String to a File using a raw fd stream - failure indicates the Presence of an
/// Error
/// @param filePath
/// @param content
/// @return llvm::Error
/// @code{.cpp}
/// if (writeString("/path/to/file.txt" , "Hello World")) {
///     // handle err...
/// }
/// @endcode
auto writeStringToFile(const llvm::StringRef &filePath,
                       const llvm::StringRef &content) -> llvm::Error;

/// @brief Write a String to a File using a raw fd stream - failure indicates the Presence of an
/// Error. Overload that performs Validation on the Input Path and Content Passed.
auto writeStringToFile(const llvm::StringRef &filePath,
                       const llvm::StringRef &content,
                       bool                   validate) -> llvm::Error;

/// @brief Read File Contents from Disk
/// @param filePath
/// @param content
/// @return llvm::Error
/// @code{.cpp}
///  std::string file_content = readFileToString("/path/to/file.txt");
/// @endcode
auto readFileToString(const llvm::Twine &path) -> std::string;

/// @brief Read File Contents as a vector<unsigned char>
/// @param filePath
/// @return std::vector<unsigned char>
auto readFileUChar(const llvm::Twine &filePath) -> llvm::Expected<std::vector<unsigned char>>;

/// @brief Get the File Paths from a Dir and Validate the Input Path
/// @param directoryPath
/// @return llvm::Expected<std::vector<std::string>>
/// @code{.cpp}
/// @endcode
auto getFilePaths(const llvm::Twine &directoryPath) -> llvm::Expected<std::vector<std::string>>;

/// @brief Stat File Info
/// @param Path
/// @return llvm::Expected<llvm::sys::fs::file_status>

auto getFileInfo(const llvm::Twine &Path) -> llvm::Expected<llvm::sys::fs::file_status>;

/// @brief Create Directories Recursively
/// @param path
/// @return llvm::Expected<bool>
auto createDirectories(const llvm::StringRef &path) -> llvm::Expected<bool>;

/// @brief  Create the Base Directory from a File Path if it does not Exist - returns True if
/// Created Successfully or the Dir Already Exists.
/// @param filePath
/// @return llvm::Expected<bool>
auto createDirectoryForFile(const llvm::Twine &filePath) -> llvm::Expected<bool>;

/// @brief Create a Qualified File Path by Parsing the Input
/// @param fileName
/// @param directory
/// @param extension
/// @param osSeparator
/// @return llvm::Expected<std::string>
/// @code{.cpp}
///   auto result = createQualifiedFilePath("testFile.png", "/path/dir" , ".txt");
///  // result "/path/dir/testFile.txt"
/// @endcode
auto createQualifiedFilePath(const llvm::StringRef &fileName,
                             const llvm::StringRef &directory,
                             const llvm::StringRef &extension,
                             char osSeparator = '/') -> llvm::Expected<std::string>;

/// @brief Delete a File at a Path
/// @param filePath
/// @return llvm::Error
auto deleteFile(const llvm::Twine &filePath) -> llvm::Error;

/// @brief Delete a Directory at a Path
/// @param directoryPath
/// @return llvm::Error
auto deleteDirectory(const llvm::Twine &directoryPath) -> llvm::Error;

/// @brief Get the Last Path Component from nested Paths
/// @param path
/// @return std::string
auto getLastPathComponent(const llvm::StringRef &path) -> std::string;

/// @brief Deletes multiple directories. This function takes a variable number of paths and attempts
/// to delete each one. It aggregates any errors encountered into a single error object that is then
/// returned.
///
/// @tparam Paths Variadic template to accept multiple path arguments of potentially differing
/// types.
/// @param paths One or more directory paths to delete.
/// @return llvm::Error Aggregated error object containing any errors that occurred during the
/// deletion process. A successful deletion results in a success error state (empty error).
///
/// @code{.cpp}
///
///     if (auto err = deleteDirectories(tempDirectory, tempBasePath, tempBaseResolvedPath)) {
///     llvm::errs() << "Error deleting directories: " << llvm::toString(std::move(err)) << '\n';
///     }
///
/// @endcode
template <typename... Paths>
auto deleteDirectories(const Paths &...paths) -> llvm::Error {
    llvm::Error aggregatedError = llvm::Error::success();
    (..., (aggregatedError = llvm::joinErrors(std::move(aggregatedError), deleteDirectory(paths))));
    return aggregatedError;
}

///  @brief Get the Err object as a str for llvm::Expected<T>
///  @param err
///  @return std::string
auto getErr(llvm::Error err) -> std::string;

/// @brief Utility for Printing llvm:Error so stderr
/// @param error
/// @return auto
/// @code{.cpp}
///    if (auto err = someFunc() ) {
///      handleError(std::move(err));
///    }
/// @endcode
inline auto handleError(llvm::Error error) {
    if (error) {
        std::string errorMsg = llvm::toString(std::move(error));
        llvm::errs() << "Error: " << errorMsg << '\n';
    }
}

/// @brief Appends the Local Timestamp to the Passed String
/// @param input
/// @return std::string
auto appendTimestamp(const std::string &input) -> std::string;

/// @brief Constructs a File Path by Extracting the Last Part of the Provided File Path if it is
/// Nested and concatenating it with the Local timestamp
/// @param inputPath
/// @return std::string
/// @code{.cpp}
///  std::string outputPathwTime = createFolderNameFromTimestampPath(inputPath);
/// @endcode
auto createFolderNameFromTimestampPath(llvm::StringRef inputPath) -> std::string;

#endif // FS_H
