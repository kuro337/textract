#include <gtest/gtest.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Error.h>
#include <src/fs.h>

namespace fs_test_constants {
    static constexpr auto tempDirectory        = "tempTestDir";
    static constexpr auto tempNonExist         = "tempTestDirNonExisting";
    static constexpr auto tempBasePath         = "baseDirOnly";
    static constexpr auto tempBaseResolvedPath = "basedirWhenFullPath";
    static constexpr auto emptyFolder          = "emptyFolder";

} // namespace fs_test_constants
using namespace fs_test_constants;

struct NoErr {};
struct Err {};

template <typename Tag, typename T>
auto ASSERT_EXPECTED(llvm::Expected<T> &expected, const std::string &errorMessage) -> void {
    if constexpr (std::is_same_v<Tag, NoErr>) {
        if (!expected) {
            llvm::Error err = expected.takeError();
            FAIL() << "Error when NoErr expected:" << errorMessage << ": "
                   << llvm::toString(std::move(err)) << '\n';
        }
    } else if constexpr (std::is_same_v<Tag, Err>) {
        if (expected) {
            FAIL() << "No Error when Err expected:" << errorMessage << '\n';
        }
    }
}
class LLVMFsTests: public ::testing::Test {
  protected:
    void SetUp() override {
        // runs per test
    }

    void TearDown() override {
        // runs per test
    }

    static void SetUpTestSuite() {
        if (!createDirectories(emptyFolder)) {
            FAIL() << "Failed to Create Empty Dir\n";
        }
    }

    static void TearDownTestSuite() {
        if (deleteDirectories(
                tempDirectory, tempBasePath, tempBaseResolvedPath, tempNonExist, emptyFolder)) {
            FAIL() << "Failed to Cleanup Temp Dirs" << '\n';
        }
    }

  public:
    const std::string validFolder = "../../images";
};

TEST_F(LLVMFsTests, GetFilePathsEmpty) {
    auto result = getFilePaths(emptyFolder);

    ASSERT_EXPECTED<NoErr>(result, "Expected valid result, but got an error");

    ASSERT_TRUE(result->empty()) << "Expected empty directory.";
}

TEST_F(LLVMFsTests, GetFilePaths) {
    auto result = getFilePaths(validFolder);
    ASSERT_EXPECTED<NoErr>(result, "Valid Folder expected no Error");

    ASSERT_EXPECTED<NoErr>(result, "Expected valid result, but got an error");

    auto emptyPathResult = getFilePaths(emptyFolder);
    ASSERT_EXPECTED<NoErr>(emptyPathResult, "Empty Dir should cause no Errors");

    auto invalidPathResult = getFilePaths("path/to/non/existing/directory");

    ASSERT_EXPECTED<Err>(invalidPathResult, "Invalid Dir should return an Error");
}

TEST_F(LLVMFsTests, GetFileInfo) {
    auto fileInfoResult      = getFileInfo(validFolder + "/imgtext.jpeg");
    auto directoryInfoResult = getFileInfo(validFolder);
    auto nonExistentResult   = getFileInfo("/path/to/non/existent/file.txt");

    ASSERT_EXPECTED<NoErr>(fileInfoResult, "File Info Valid");
    ASSERT_EXPECTED<NoErr>(directoryInfoResult, "Dir Info Valid");
    ASSERT_EXPECTED<Err>(nonExistentResult, "Non Existent should Return an Error");
}

TEST_F(LLVMFsTests, DirectoryCreationTests) {
    auto existingDirResult = createDirectories(validFolder);
    auto newDirResult      = createDirectories(tempBasePath);
    auto fullPath          = llvm::Twine(tempBaseResolvedPath) + "/mock.txt";
    auto fileInNewDir      = createDirectoryForFile(fullPath);
    auto fileInExistingDir = createDirectoryForFile(fullPath);

    ASSERT_EXPECTED<NoErr>(existingDirResult, "Creating existing directory should not error");
    ASSERT_EXPECTED<NoErr>(newDirResult, "Creating new directory should succeed");
    ASSERT_EXPECTED<NoErr>(fileInNewDir, "Should Extract Base dir from full Path and create Dir");
    ASSERT_EXPECTED<NoErr>(fileInExistingDir, "Creating dir for existing files should not err");

    ASSERT_TRUE(*existingDirResult);
    ASSERT_TRUE(*newDirResult);
    ASSERT_TRUE(*fileInNewDir);
    ASSERT_TRUE(*fileInExistingDir);
}

TEST_F(LLVMFsTests, CreateQualifiedFilePath) {
    auto result = createQualifiedFilePath("testFile.png", tempDirectory, ".txt");

    ASSERT_EXPECTED<NoErr>(result, "Failed to create path");
    ASSERT_TRUE(llvm::sys::fs::exists(tempDirectory)) << "File should exist at: " << *result;
}

auto createQualifiedFilePatha(const std::string &fileName,
                              const std::string &directory,
                              const char         path_separator = '/') -> std::string {
    if (!directory.empty() && !llvm::sys::fs::exists(directory) && !createDirectories(directory)) {
        throw std::runtime_error("Failed to Create Directory at: " + directory);
    }

    std::string outputFilePath = directory;
    if (!directory.empty() && directory.back() != path_separator) {
        outputFilePath += path_separator;
    }

    std::size_t lastSlash = fileName.find_last_of("/\\");
    std::size_t lastDot   = fileName.find_last_of('.');
    std::string filename  = fileName.substr(lastSlash + 1, lastDot - lastSlash - 1);

    outputFilePath += filename + ".txt";

    return outputFilePath;
}

TEST_F(LLVMFsTests, CreateQualifiedFilePathExistingDirectory) {
    auto result   = createQualifiedFilePath("image.png", tempDirectory, ".txt");
    auto old      = createQualifiedFilePatha("image.png", tempDirectory);
    auto fullPath = llvm::Twine(tempDirectory) + "/image.txt";

    ASSERT_EXPECTED<NoErr>(result, "tempdir/image.png returned an unexpected Error");
    EXPECT_TRUE(llvm::sys::fs::exists(tempDirectory));
    ASSERT_EQ(old, *result);
    EXPECT_EQ(*result, fullPath.str());
}

TEST_F(LLVMFsTests, CreateQualifiedFilePathNonExistingDirectory) {
    auto result   = createQualifiedFilePath("document.pdf", tempNonExist, ".txt");
    auto old      = createQualifiedFilePatha("document.pdf", tempNonExist);
    auto fullPath = llvm::Twine(tempNonExist) + "/document.txt";

    ASSERT_EXPECTED<NoErr>(result, "Dir expected to be Created for a non-existing Path");
    ASSERT_EQ(old, *result);
    EXPECT_EQ(*result, fullPath.str());
    EXPECT_TRUE(llvm::sys::fs::exists(tempNonExist));
}

TEST_F(LLVMFsTests, CreateQualifiedFilePathExtensionChange) {
    auto result = createQualifiedFilePath("archive.tar.gz", tempDirectory, ".bak");

    ASSERT_EXPECTED<NoErr>(result, "Mock Path for .tar.gz Failure");
    EXPECT_EQ(*result, "tempTestDir/archive.tar.bak");
    EXPECT_TRUE(llvm::sys::fs::exists(tempDirectory));
}

auto main(int argc, char **argv) -> int {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
