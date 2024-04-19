// fs.h
#ifndef FS_H
#define FS_H

#include <llvm/Support/FileSystem.h>

auto getFilePaths(const llvm::Twine &directoryPath) -> llvm::Expected<std::vector<std::string>>;

auto getFileInfo(const llvm::Twine &Path) -> llvm::Expected<llvm::sys::fs::file_status>;

auto createDirectories(const llvm::Twine &path) -> llvm::Expected<bool>;

auto createDirectoryForFile(const llvm::Twine &filePath) -> llvm::Expected<bool>;

#endif // FS_H
