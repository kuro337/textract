#include "fs.h"
#include "util.h"
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

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
                       const llvm::StringRef &content) -> llvm::Error {
    std::error_code      ERR;
    llvm::raw_fd_ostream outstream(filePath.str(), ERR, llvm::sys::fs::OF_None);

    if (ERR) {
        std::string errMsg = "Failed to open file for writing: " + filePath.str();
        return llvm::make_error<llvm::StringError>(std::move(errMsg), ERR);
    }
    outstream << content;
    outstream.close();

    if (outstream.has_error()) {
        std::string errMsg = "Failed to write to file: " + filePath.str();
        return llvm::make_error<llvm::StringError>(std::move(errMsg), outstream.error());
    }

    return llvm::Error::success();
}

/// @brief Write a String to a File using a raw fd stream - failure indicates the Presence of an
/// Error. Overload that performs Validation on the Input Path and Content Passed.
auto writeStringToFile(const llvm::StringRef &filePath,
                       const llvm::StringRef &content,
                       bool                   validate) -> llvm::Error {
    if (validate && hasSpecialChars(filePath)) {
        std::string errMsg = "Invalid Characters Detected in File Path: " + filePath.str();
        return llvm::make_error<llvm::StringError>(
            errMsg, std::make_error_code(std::errc::invalid_argument));
    }

    return writeStringToFile(filePath, content);
}

/// @brief Get the File Paths from a Dir and Validate the Input Path
/// @param directoryPath
/// @return llvm::Expected<std::vector<std::string>>
/// @code{.cpp}
/// @endcode
auto getFilePaths(const llvm::Twine &directoryPath) -> llvm::Expected<std::vector<std::string>> {
    std::error_code                   ERR;
    llvm::sys::fs::directory_iterator dirIt(directoryPath, ERR);

    if (ERR) {
        return llvm::make_error<llvm::StringError>(ERR.message(), ERR);
    }

    llvm::sys::fs::directory_iterator dirEnd;
    std::vector<std::string>          filePaths;

    for (; dirIt != dirEnd && !ERR; dirIt.increment(ERR)) {
        if (ERR) {
            return llvm::make_error<llvm::StringError>(ERR.message(), ERR);
        }
        filePaths.push_back(dirIt->path());
    }

    return filePaths;
}

/// @brief Read File Contents from Disk
/// @param filePath
/// @param content
/// @return llvm::Error
/// @code{.cpp}
///  std::string file_content = readFileToString("/path/to/file.txt");
/// @endcode
auto readFileToString(const llvm::Twine &path) -> std::string {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> bufferOrErr =
        llvm::MemoryBuffer::getFile(path);

    if (std::error_code ERR = bufferOrErr.getError()) {
        return "";
    }

    llvm::MemoryBuffer &buffer      = *bufferOrErr.get();
    llvm::StringRef     fileContent = buffer.getBuffer();

    return fileContent.str();
}

/// @brief Read File Contents as a vector<unsigned char>
/// @param filePath
/// @return std::vector<unsigned char>
auto readFileUChar(const llvm::Twine &filePath) -> llvm::Expected<std::vector<unsigned char>> {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> bufferOrErr =
        llvm::MemoryBuffer::getFile(filePath);

    if (std::error_code ERR = bufferOrErr.getError()) {
        return llvm::make_error<llvm::StringError>(ERR.message(), ERR);
    }

    llvm::MemoryBuffer &buffer      = *bufferOrErr.get();
    llvm::StringRef     fileContent = buffer.getBuffer();
    return std::vector<unsigned char>(fileContent.bytes_begin(), fileContent.bytes_end());
}

auto readBytesFromFile(const std::string &filename) -> std::vector<unsigned char> {
#ifdef _DEBUGFILEIO
    sout << "Converting to char* " << filename << std::endl;
#endif
    auto fileContentOrErr = readFileUChar(filename);
    if (!fileContentOrErr) {
        llvm::errs() << "Error: " << llvm::toString(fileContentOrErr.takeError()) << "\n";
        throw std::runtime_error("Failed to read file: " + filename);
    }
    return fileContentOrErr.get();
}

/// @brief Stat File Info
/// @param Path
/// @return llvm::Expected<llvm::sys::fs::file_status>
auto getFileInfo(const llvm::Twine &Path) -> llvm::Expected<llvm::sys::fs::file_status> {
    llvm::sys::fs::file_status Status;
    if (std::error_code ERR = llvm::sys::fs::status(Path, Status)) {
        return llvm::make_error<llvm::StringError>(ERR.message(), ERR);
    }
    return Status;
}

/// @brief Create Directories Recursively
/// @param path
/// @return llvm::Expected<bool>
auto createDirectories(const llvm::StringRef &path) -> llvm::Expected<bool> {
    if (path.empty()) {
        return llvm::make_error<llvm::StringError>(
            "Empty Path passed to create directory", llvm::inconvertibleErrorCode());
    }

    if (auto err = llvm::sys::fs::create_directories(path); err) {
        return llvm::make_error<llvm::StringError>(err.message(), err);
    }
    return true;
}

/// @brief  Create the Base Directory from a File Path if it does not Exist - returns True if
/// Created Successfully or the Dir Already Exists.
/// @param filePath
/// @return llvm::Expected<bool>
auto createDirectoryForFile(const llvm::Twine &filePath) -> llvm::Expected<bool> {
    llvm::SmallString<256> fullPathStorage;
    filePath.toVector(fullPathStorage);

    llvm::StringRef fullPathRef(fullPathStorage);

    llvm::StringRef directoryPathRef = llvm::sys::path::parent_path(fullPathRef);

    // Handle case when input passed is directly a Dir Path - Create Dir from filePath
    if (directoryPathRef.empty()) {
        llvm::errs() << "No directory part in path; assuming current directory. Consider using "
                        "createDirectories() instead if only a Dir Path is expected.\n";
        return createDirectories(filePath.getSingleStringRef());
    }

    llvm::SmallString<256> directoryPath(directoryPathRef);

    if (llvm::sys::fs::exists(directoryPath)) {
        return true;
    }

    if (auto err = llvm::sys::fs::create_directories(directoryPath); err) {
        llvm::errs() << "Error creating directory '" << directoryPath << "': " << err.message()
                     << "\n";
        return llvm::make_error<llvm::StringError>(
            llvm::formatv("Error creating directory {0}: {1}", directoryPath.str(), err.message()),
            err);
    }

    return true;
}

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
                             const llvm::StringRef &extension = ".txt",
                             const char             osSeparator) -> llvm::Expected<std::string> {
    llvm::SmallString<256> outputFilePath;
    llvm::sys::path::append(outputFilePath, directory);

    if (!outputFilePath.empty() && outputFilePath.back() != osSeparator) {
        outputFilePath.push_back(osSeparator);
    }

    llvm::SmallString<256> fullFilename = llvm::sys::path::filename(fileName);
    llvm::sys::path::replace_extension(fullFilename, extension);
    llvm::sys::path::append(outputFilePath, fullFilename);

    return std::string(outputFilePath.str());
}

/// @brief Delete a File at a Path
/// @param filePath
/// @return llvm::Error
auto deleteFile(const llvm::Twine &filePath) -> llvm::Error {
    if (auto err = llvm::sys::fs::remove(filePath)) {
        return llvm::make_error<llvm::StringError>("Failed to delete file: " + filePath.str(), err);
    }
    return llvm::Error::success();
}

/// @brief Delete a Directory at a Path
/// @param directoryPath
/// @return llvm::Error
auto deleteDirectory(const llvm::Twine &directoryPath) -> llvm::Error {
    if (auto err = llvm::sys::fs::remove_directories(directoryPath)) {
        return llvm::make_error<llvm::StringError>(
            "Failed to delete directory: " + directoryPath.str(), err);
    }
    return llvm::Error::success();
}

/// @brief Get the Last Path Component from nested Paths
/// @param path
/// @return std::string
auto getLastPathComponent(const llvm::StringRef &path) -> std::string {
    std::size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 1).str();
    }
    return path.str();
}

/// @brief Appends the Local Timestamp to the Passed String
/// @param input
/// @return std::string
auto appendTimestamp(const std::string &input) -> std::string {
    constexpr int bufsize = 20;

    auto    now      = std::chrono::system_clock::now();
    auto    time     = std::chrono::system_clock::to_time_t(now);
    std::tm timestmp = *std::localtime(&time);

    std::array<char, bufsize> buffer {};
    std::strftime(buffer.data(), buffer.size(), "%Y%m%d_%H%M%S", &timestmp);
    std::string timeStr(buffer.data());

    std::string result = llvm::formatv("{0}_{1}", input, timeStr);

    return result;
}

/// @brief Constructs a File Path by Extracting the Last Part of the Provided File Path if it is
/// Nested and concatenating it with the Local timestamp
/// @param inputPath
/// @return std::string
/// @code{.cpp}
///  std::string outputPathwTime = createFolderNameFromTimestampPath(inputPath);
/// @endcode
auto createFolderNameFromTimestampPath(const llvm::StringRef inputPath) -> std::string {
    return appendTimestamp(getLastPathComponent(inputPath));
}

///  @brief Check if the Path exists
///  @param path
///  @return bool
auto file_exists(const llvm::Twine &path) -> bool { return llvm::sys::fs::exists(path); }

///  @brief Check if the Path is a Valid Directory
///  @param directoryPath
///  @return bool
auto is_dir(const llvm::Twine &directoryPath) -> bool {
    return llvm::sys::fs::is_directory(directoryPath);
}

/// @brief -1 NonExistent , 0:File , 1:Dir
/// @param unknown_path
/// @return int
/// @code{.cpp}
/// @endcode
int path_stat(const llvm::Twine &unknown_path) {
    if (!llvm::sys::fs::exists(unknown_path)) {
        return -1;
    }
    if (llvm::sys::fs::is_directory(unknown_path)) {
        return 1;
    };
    return 0;
}

///  @brief Get the Err object as a str for llvm::Expected<T>
///  @param err
///  @return std::string
auto getErr(llvm::Error err) -> std::string { return llvm::toString(std::move(err)); }
