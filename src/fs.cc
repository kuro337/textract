
#include "fs.h"
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
auto getFileInfo(const llvm::Twine &Path) -> llvm::Expected<llvm::sys::fs::file_status> {
    llvm::sys::fs::file_status Status;
    if (std::error_code ERR = llvm::sys::fs::status(Path, Status)) {
        return llvm::make_error<llvm::StringError>(ERR.message(), ERR);
    }
    return Status;
}

auto createDirectories(const llvm::Twine &path) -> llvm::Expected<bool> {
    if (auto err = llvm::sys::fs::create_directories(path); err) {
        return llvm::make_error<llvm::StringError>(err.message(), err);
    }
    return true;
}

auto createDirectoryForFile(const llvm::Twine &filePath) -> llvm::Expected<bool> {
    llvm::SmallString<256> fullPathStorage;
    filePath.toVector(fullPathStorage);

    llvm::StringRef fullPathRef(fullPathStorage);

    llvm::StringRef directoryPathRef = llvm::sys::path::parent_path(fullPathRef);

    llvm::SmallString<256> directoryPath(directoryPathRef);

    if (llvm::sys::fs::exists(directoryPath)) {
        return true;
    }

    if (auto err = llvm::sys::fs::create_directories(directoryPath); err) {
        llvm::errs() << "Error creating directory '" << directoryPath << "': " << err.message() << "\n";
        return llvm::make_error<llvm::StringError>(
            llvm::formatv("Error creating directory {0}: {1}", directoryPath.str(), err.message()), err);
    }

    return true;
}