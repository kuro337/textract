
// cli.h
#ifndef CLI_H
#define CLI_H

#include "textract.h"
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

static inline auto &sout = llvm::outs();
static inline auto &serr = llvm::errs();

/// @brief Read Data from stdin and Close stdin - returns a Stack Allocated String
/// @return llvm::Expected<llvm::SmallString<256>>
inline auto readStdin() -> llvm::Expected<llvm::SmallString<256>> {
    constexpr size_t numCharsToRead = 4;
    std::string      inputData(numCharsToRead, '\0');

    // Read numCharsToRead characters from stdin
    if (read(STDIN_FILENO, inputData.data(), numCharsToRead) == -1) {
        llvm::errs() << "Error reading from stdin\n";
        return llvm::createStringError(
            std::make_error_code(std::errc::io_error), "Error reading from stdin");
    }

    // Close the stdin file descriptor
    if (close(STDIN_FILENO) == -1) {
        llvm::errs() << "Error closing stdin\n";
        return llvm::createStringError(
            std::make_error_code(std::errc::io_error), "Error closing stdin");
    }

    auto inputBuffer =
        llvm::MemoryBuffer::getMemBuffer(inputData, "stdin", /*RequiresNullTerminator=*/false);

    llvm::StringRef bufferData = inputBuffer->getBuffer().trim();

    llvm::SmallString<256> sindata(bufferData.lower());

    return sindata;
}

/// @brief Validate User Input to Proceed
/// @return true
/// @return false
inline auto validateYesNo() -> bool {
    if (auto dataOrErr = readStdin(); dataOrErr) {
        auto sindata = dataOrErr.get();
        return sindata == "yes" || sindata == "y";
    }
    return false;
}

/// @brief Helper for User Input Validation
/// @return true
/// @return false
inline auto userConfirmation(const llvm::Twine &inputDir, const llvm::Twine &outputDir) -> bool {
    auto files = getFilePaths(inputDir);

    soutfmt("Found {0} files in directory {1}.\nProceed with converting images to text to "
            "destination {2}?\n (yes/no)\n",
            files->size(),
            inputDir,
            outputDir);

    sout.flush();

    return validateYesNo();
}

/// @brief Determine Behavior according to stdin Data and appropriately proceed with the Conversions
/// @param inputPath
/// @param outputPath
inline void process(llvm::StringRef inputPath, llvm::StringRef outputPath) {
    if (!file_exists(inputPath)) {
        serr << "Error: Invalid input path provided.\n";
        return;
    }

    std::unique_ptr<imgstr::ImgProcessor> app = std::make_unique<imgstr::ImgProcessor>();

    if (is_dir(inputPath)) {
        if (outputPath.empty()) {
            auto outputPathwTime = createFolderNameFromTimestampPath(inputPath);

            if (!userConfirmation(inputPath, outputPathwTime)) {
                sout << "Processing Cancelled by user.\n";
                return;
            }

            app->simpleProcessDir(inputPath.str(), outputPathwTime);
            return;
        }

        app->simpleProcessDir(inputPath.str(), outputPath.str());
        return;
    }

    if (!outputPath.empty()) {
        HandleError<Throw>(app->processSingleImage(inputPath.str(), outputPath.str()));
        return;
    }

    auto content = app->getTextFromImage(inputPath.str());

    sout << content << '\n';
}

#endif // CLI_H