#include "cli.h"
#include "fs.h"
#include "logger.h"
#include "util.h"
#include <future>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace sin_test_constants {
    static constexpr auto tempDirectory = "temporary_dir";
}

using namespace sin_test_constants;

class SinTest: public ::testing::Test {
  public:
    static std::string tempDir;
    static int         argc;
    static char      **argv;
};

std::string SinTest::tempDir;
int         SinTest::argc = 0;
// char      **SinTest::argv = nullptr;
char **SinTest::argv;

// Definition of static members
// std::vector<std::string> SinTest::static_memb;

TEST_F(SinTest, BasicAssertions) {
    // stdin
    ASSERT_EQ(5, 5);
}

TEST_F(SinTest, BasicTest) {
    // implement pure test
    ASSERT_EQ(1, 1);
}

bool isDirectory(const std::string &path) {
    struct stat s {};
    if (stat(path.c_str(), &s) == 0) {
        return (s.st_mode & S_IFDIR) != 0;
    }
    return false;
}

struct KFile {
    std::string path;
    KFile(std::string &p): path(p) {}
};

struct KDir {
    std::string              path;
    std::vector<std::string> dir_files;

    KDir(std::string p): path(std::move(p)) {}

    // Synchronous method to populate dir_files
    void populateDirFiles() {
        auto paths = getFilePaths(path);
        if (paths) {
            dir_files = paths.get();
        }
    }

    // Asynchronous method to populate dir_files
    std::future<void> populateDirFilesAsync() {
        return std::async(std::launch::async, &KDir::populateDirFiles, this);
    }
};

struct KNonExistent {
    std::string path;
    KNonExistent(std::string p): path(std::move(p)) {}
};

namespace llvm {
    template <>
    struct format_provider<KFile> {
        static void format(const KFile &value, raw_ostream &stream, StringRef style) {
            stream << value.path;
        }
    };

    template <>
    struct format_provider<KDir> {
        static void format(const KDir &value, raw_ostream &stream, StringRef style) {
            stream << Ansi::FOLDER_ICON << " " << value.path << "\nFiles: ";
            for (const auto &file: value.dir_files) {
                stream << file << " ";
            }
        }
    };

    template <>
    struct format_provider<KNonExistent> {
        static void format(const KNonExistent &value, raw_ostream &stream, StringRef style) {
            stream << value.path;
        }
    };
} // namespace llvm

struct KFs {
    std::vector<KFile>        files;
    std::vector<KDir>         dirs;
    std::vector<KNonExistent> nonExistent;

    /// @brief Print Result Metadata about Valid Paths of Files/Dirs
    void print() {
        if (files.size() > 0) {
            soutfmt("{0} {1} Files:", Ansi::SUCCESS_TICK_RGB, files.size());

            soutfmt("{0:$[  \n]}", llvm::make_range(files.begin(), files.end()));
        }

        if (dirs.size() > 0) {
            soutfmt("{0} {1} Directories:", Ansi::SUCCESS_TICK_RGB, dirs.size());
            soutfmt("{0:$[  \n]}", llvm::make_range(dirs.begin(), dirs.end()));
        }

        if (nonExistent.size() > 0) {
            serrfmt("{0} {1} Invalid Paths:", Ansi::FAILURE_CROSS, nonExistent.size());
            serrfmt("{0:$[  \n]}", llvm::make_range(nonExistent.begin(), nonExistent.end()));
        }
    }

    /// @brief Get stdin confirmation whether to Proceed or not
    /// @return true
    /// @return false
    [[nodiscard]] bool confirm() const {
        if (files.size() > 0 || dirs.size() > 0) {
            sout << "Proceed with Processing Files?\n";
            auto res = validateYesNo();
            if (res) {
                sout << "Confirmed\n";
                return true;
            }
            sout << "Aborting\n";
            return false;
        }
        serr << "No valid paths detected.\n";
        return false;
    }
};

enum PathType { FILE_PATH, DIRECTORY_PATH, NON_EXISTENT_PATH };

PathType getPathType(const std::string &path) {
    struct stat s {};
    if (stat(path.c_str(), &s) == 0) {
        if ((s.st_mode & S_IFDIR) != 0) {
            return DIRECTORY_PATH;
        }
        return FILE_PATH;
    }
    return NON_EXISTENT_PATH;
}

std::future<KFs> inputAsync(int argc, char **argv) {
    return std::async(std::launch::async, [argc, argv]() {
        KFs result;

        std::cout << "argc: " << argc << std::endl;
        for (int i = 0; i < argc; ++i) {
            if (argv[i] != nullptr) {
                std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
            } else {
                std::cout << "argv[" << i << "]: nullptr" << std::endl;
            }
        }

        if (argc < 2) {
            std::cerr << "No input paths provided" << std::endl;
            return result;
        }

        std::vector<std::future<void>> futures;

        for (int i = 1; i < argc; ++i) {
            if (argv[i] == nullptr) {
                std::cerr << "Null argument at index " << i << std::endl;
                continue;
            }

            std::string path     = argv[i];
            PathType    pathType = getPathType(path);
            switch (pathType) {
                case DIRECTORY_PATH:
                    result.dirs.emplace_back(path);
                    futures.push_back(result.dirs.back().populateDirFilesAsync());
                    break;
                case FILE_PATH:
                    result.files.emplace_back(path);
                    break;
                case NON_EXISTENT_PATH:
                    result.nonExistent.emplace_back(path);
                    break;
            }
        }

        for (auto &future: futures) {
            future.get();
        }

        return result;
    });
}

std::vector<int> cinput(int argc, char **argv) {
    std::vector<int> result;
    if (argc < 2) {
        std::cerr << "No input paths provided" << std::endl;
        return result;
    }

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            std::cerr << "Null argument at index " << i << std::endl;
            continue;
        }

        std::string path     = argv[i];
        auto        pathType = path_stat(path);
        switch (pathType) {
            case 1:
                sout << "Dir: " << path << '\n';
                result.emplace_back(1);
                break;
            case 0:
                sout << "File: " << path << '\n';
                result.emplace_back(0);
                break;
            case -1:
                sout << "Non Existent: " << path << '\n';
                result.emplace_back(-1);
                break;
            default:
                result.emplace_back(2);
        }
    }

    return result;
}

KFs input(int argc, char **argv) {
    KFs result;
    if (argc < 2) {
        std::cerr << "No input paths provided" << std::endl;
        return result;
    }

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            std::cerr << "Null argument at index " << i << std::endl;
            continue;
        }

        std::string path     = argv[i];
        PathType    pathType = getPathType(path);
        switch (pathType) {
            case DIRECTORY_PATH:
                result.dirs.emplace_back(path);
                break;
            case FILE_PATH:
                result.files.emplace_back(path);
                break;
            case NON_EXISTENT_PATH:
                result.nonExistent.emplace_back(path);
                break;
        }
    }

    return result;
}

TEST_F(SinTest, FileDirInp) {
    auto futureFs   = inputAsync(argc, argv);
    KFs  fileSystem = futureFs.get();
    fileSystem.print();
    //    auto valid = fileSystem.confirm();
}

// ctest -R SinTest.FileDirInp
int main(int argc, char **argv) {
    SinTest::argc = argc;
    SinTest::argv = argv;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
