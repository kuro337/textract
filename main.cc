#include <cli.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <textract.h>

inline void printHelp() {
    sout << "Usage:\n";
    sout << "  ./main <inputDirPath> [<outputDirPath>]\n";
    sout << "  ./main <inputFilePath> [<outputFilePath>]\n";
}

auto main(int argc, char **argv) -> int {
    printSystemInfo();

    if (argc < 2) {
        printHelp();
        return 1;
    }

    llvm::StringRef inputPath(argv[1]);

    llvm::StringRef outputPath;

    if (argc >= 3) {
        outputPath = argv[2];
    }

    process(inputPath, outputPath);

    sout << "Processing completed successfully.\n";

    return 0;
}

/*

./textract_static /Users/kuro/Documents/Code/Cpp/image/imgapp/images

./textract_static /Users/kuro/Documents/Code/Cpp/image/imgapp/images/screenshot.png





*/
