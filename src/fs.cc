#include "fs.h"
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/Path.h>

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

auto getFilePathsReal(const llvm::Twine &directoryPath)
    -> llvm::Expected<std::vector<std::string>> {
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

auto getFileInfo(const llvm::Twine &Path) -> llvm::Expected<llvm::sys::fs::file_status> {
    llvm::sys::fs::file_status Status;
    if (std::error_code ERR = llvm::sys::fs::status(Path, Status)) {
        return llvm::make_error<llvm::StringError>(ERR.message(), ERR);
    }
    return Status;
}

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

auto deleteFile(const llvm::Twine &filePath) -> llvm::Error {
    if (auto err = llvm::sys::fs::remove(filePath)) {
        return llvm::make_error<llvm::StringError>("Failed to delete file: " + filePath.str(), err);
    }
    return llvm::Error::success();
}

auto deleteDirectory(const llvm::Twine &directoryPath) -> llvm::Error {
    if (auto err = llvm::sys::fs::remove_directories(directoryPath)) {
        return llvm::make_error<llvm::StringError>(
            "Failed to delete directory: " + directoryPath.str(), err);
    }
    return llvm::Error::success();
}

/**
 * @brief Get the Err object as a str for llvm::Expected<T>
 * @param err
 * @return std::string
 */
auto getErr(llvm::Error err) -> std::string { return llvm::toString(std::move(err)); }
