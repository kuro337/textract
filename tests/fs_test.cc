#include <gtest/gtest.h>
#include <llvm/Support/Error.h>
#include <src/fs.h>

template <typename T>
auto ErrorExists(llvm::Expected<T> &expected, const std::string &errorMessage) -> bool {
    if (!expected) {
        llvm::Error err = expected.takeError();
        std::cerr << "Assert Failure:" << errorMessage << ": " << llvm::toString(std::move(err)) << '\n';
        return true;
    }
    return false;
}

class LLVMFsTests: public ::testing::Test {
  protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

  public:
    const std::string validFolder = "../../images";
    const std::string emptyFolder = "../../empty";
};

TEST_F(LLVMFsTests, GetFilePathsEmpty) {
    auto result = getFilePaths(emptyFolder);

    ASSERT_FALSE(ErrorExists(result, "Expected valid result, but got an error"));

    if (!result) {
        llvm::Error err = result.takeError();
        llvm::consumeError(std::move(err));
        FAIL() << "Expected valid result, but got error.";
    }
    ASSERT_TRUE(result->empty()) << "Expected empty directory.";
}

TEST_F(LLVMFsTests, GetFilePaths) {
    auto result = getFilePaths(validFolder);
    ASSERT_FALSE(ErrorExists(result, "Valid Folder expected no Error"));

    auto emptyPathResult = getFilePaths(emptyFolder);
    ASSERT_FALSE(ErrorExists(emptyPathResult, "Empty Dir should cause no Errors"));

    auto invalidPathResult = getFilePaths("path/to/non/existing/directory");

    ASSERT_TRUE(ErrorExists(invalidPathResult, "Invalid Dir should return an Error"));
}

TEST_F(LLVMFsTests, GetFileInfo) {
    auto fileInfoResult = getFileInfo(validFolder + "/imgtext.jpeg");
    ASSERT_FALSE(ErrorExists(fileInfoResult, "File Info Valid"));

    auto directoryInfoResult = getFileInfo(validFolder);
    ASSERT_FALSE(ErrorExists(directoryInfoResult, "Dir Info Valid"));

    auto nonExistentResult = getFileInfo("/path/to/non/existent/file.txt");
    ASSERT_TRUE(ErrorExists(nonExistentResult, "Non Existent should Return an Error"));
}

TEST_F(LLVMFsTests, DirectoryCreationTests) {
    auto existingDirResult = createDirectories(validFolder);

    ASSERT_FALSE(ErrorExists(existingDirResult, "Creating existing directory should not error"));
    ASSERT_TRUE(*existingDirResult);

    auto newDirResult = createDirectories("llvmtest");

    ASSERT_FALSE(ErrorExists(newDirResult, "Creating new directory should succeed"));
    ASSERT_TRUE(*newDirResult);

    auto fileInNewDir = createDirectoryForFile("llvmfiletest/mock.txt");
    ASSERT_FALSE(ErrorExists(fileInNewDir, "Creating directory for new file should succeed"));
    ASSERT_TRUE(*fileInNewDir);

    auto fileInExistingDir = createDirectoryForFile("llvmfiletest/mock.txt");
    ASSERT_FALSE(ErrorExists(fileInExistingDir, "Creating directory for an existing file should still succeed"));
    ASSERT_TRUE(*fileInExistingDir);
}

auto main(int argc, char **argv) -> int {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
