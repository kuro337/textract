<br/>
<br/>

<div align="left">
  <img alt="textract logo" height="75px" src="assets/logo.png">
</div>

<br/>
<br/>

# textract

<br/>
<br/>

_Single Header High Performance_ **C++ Image Processing** Library to read content from Images and transform Images to text files.

<br/>

<hr/>

<br/>

Build from Source using **CMake**

#### Dependencies

<br/>

```bash
brew install openssl leptonica tesseract libomp
```

<br/>

#### Build

<br/>

```bash

cd textract && mkdir build && cd build
cmake ..
make

# or use the build script to build for all releases and run tests
./build_all.sh --all --tests

## Alternative Compilers ##

# using LLVM and Clang++ directly
cmake -DCMAKE_CXX_COMPILER=/path/to/clang++ -DCMAKE_C_COMPILER=/path/to/clang ..
make

# getting clang++ and clang paths
echo $(brew --prefix llvm)/bin/clang++
echo $(brew --prefix llvm)/bin/clang

# to build all releases
./build_all.sh

```

<br/>

[Linux (AArch64, x86_64)](docs/linux.md)

<br/>

## Design

<br/>

#### leptonica and Tesseract

For Processing images and using _Tesseract OCR_ to extract text from Images.

<hr/>

#### OpenSSL

For generating **SHA256** hashes from Image bytes and metadata.

<hr/>

#### OpenMP

To provide **parallelization** on systems for processing.

<hr/>

#### Folly

_textract_ uses Folly's **AtomicUnorderedInsertMap** for a fast Cache implementation to provide wait free parallel access to the Cache

[Folly](https://github.com/facebook/folly)::[AtomicUnorderedInsertMap](https://github.com/facebook/folly/blob/main/folly/AtomicUnorderedMap.h)

<hr>

<br/>

## Usage

<br/>

Process Images and get their textual content

<br/>

```cpp
#include <textract/textract>

int main() {
    imgstr::ImgProcessor app = imgstr::ImgProcessor();

    std::vector<std::string> results = app.processImages("cs101_notes.png","bio.jpeg");

    app.writeImageTextOut("cs101_notes.png", "cs_notes.txt");



    app.createPDF("transcript_img.png");    /* Create searchable PDF's from images */

    return 0;
}

```

<br/>

Process all valid Image files from a directory and create text files

<br/>

```cpp
#include <textract/textract>

int main() {

    /* Process 10000+ images with parallelism */

    imgstr::ImageTranslator app = imgstr::ImageTranslator(10000);

    app.setCores(6);

    app.setMode(ImgMode::document);

    app.processImagesDir("/path/to/dir");

    app.getResults();

    return 0;
}

```

<hr>

<br/>

### In Memory Cache Performance

<br/>

```bash

Debug -O0

============================================================================
/textract/benchmarks/cache_benchmark.cc       relative    time/iter  iters/s
============================================================================
UnorderedMapMutexedSingleThreaded                          60.70ns    16.48M
UnorderedMapMutexedMultiThreaded                          771.14ns     1.30M
UnorderedMapMutexedMaxThreads                               1.62us   618.92K
ConcurrentHashMapSingleThreaded                           255.11ns     3.92M
ConcurrentHashMapMultiThreaded                            978.99ns     1.02M
ConcurrentHashMapMaxThreads                                 3.92us   254.82K
ConcurrentHashMapComplexSingleThreaded                    394.03ns     2.54M
ConcurrentHashMapComplexMultiThreaded                       1.68us   595.64K
ConcurrentHashMapComplexMaxThreads                         13.05us    76.62K
AtomicUnorderedMapSingleThreaded                           38.80ns    25.77M
AtomicUnorderedMapMultiThreaded                            75.43ns    13.26M
AtomicUnorderedMapMaxThreads                              451.00ns     2.22M
AtomicUnorderedMapComplexSingleThreaded                   256.42ns     3.90M
AtomicUnorderedMapComplexMultiThreaded                      1.68us   593.57K
AtomicUnorderedMapComplexMaxThreads                         8.76us   114.17K
============================================================================


Fast Clang -Ofast

============================================================================
[...]/imgapp/benchmarks/cache_benchmark.cc     relative  time/iter   iters/s
============================================================================
UnorderedMapMutexedSingleThreaded                          61.15ns    16.35M
UnorderedMapMutexedMultiThreaded                          781.00ns     1.28M
UnorderedMapMutexedMaxThreads                               1.62us   616.71K
ConcurrentHashMapSingleThreaded                           215.90ns     4.63M
ConcurrentHashMapMultiThreaded                            864.01ns     1.16M
ConcurrentHashMapMaxThreads                                 3.50us   285.73K
ConcurrentHashMapComplexSingleThreaded                    282.30ns     3.54M
ConcurrentHashMapComplexMultiThreaded                       1.32us   757.35K
ConcurrentHashMapComplexMaxThreads                         14.95us    66.90K
AtomicUnorderedMapSingleThreaded                           41.16ns    24.30M
AtomicUnorderedMapMultiThreaded                            81.05ns    12.34M
AtomicUnorderedMapMaxThreads                              440.18ns     2.27M
AtomicUnorderedMapComplexSingleThreaded                   127.24ns     7.86M
AtomicUnorderedMapComplexMultiThreaded                    511.31ns     1.96M
AtomicUnorderedMapComplexMaxThreads                         6.31us   158.55K
============================================================================

```

<br/>

It can be seen **AtomicUnorderedInsertMap** is over **8x** faster than the Concurrent HashMap.

[AtomicUnorderedInsertMap](https://github.com/facebook/folly/blob/main/folly/AtomicUnorderedMap.h) provides an overview of the tradeoffs.

<hr>

<br/>

**Note**

_These are for reference and will vary depending on:_

- cpu arch
- release Mode
- number of cores
- types of images

<br/>

**Average Latencies** for reference using **4 cores** and batches of **10mb** upto **4gb**

<hr>

Document Images to Text Generations: **0.423081 ms**

Complex Images to Text Generations: **0.674621 ms**

Text Document to Searchable PDF Conversion: **0.381223 ms**

<hr>

<br/>

`Latest Test Suite Run`

```bash
      Start  1: ImageTest.MutexNullIfNoWrite
 1/37 Test  #1: ImageTest.MutexNullIfNoWrite ..............................   Passed    0.02 sec
      Start  2: ImageTest.MutexLazyInitialization
 2/37 Test  #2: ImageTest.MutexLazyInitialization .........................   Passed    0.02 sec
      Start  3: ImageTest.UpdateWriteMetadata
 3/37 Test  #3: ImageTest.UpdateWriteMetadata .............................   Passed    0.02 sec
      Start  4: ImageTest.ReadWriteMetadataBeforeWrite
 4/37 Test  #4: ImageTest.ReadWriteMetadataBeforeWrite ....................   Passed    0.02 sec
      Start  5: ImageConcurrentWriteTest.ConcurrentWriteAttempts
 5/37 Test  #5: ImageConcurrentWriteTest.ConcurrentWriteAttempts ..........   Passed    0.02 sec
      Start  6: ConstTests.BasicAssertions
 6/37 Test  #6: ConstTests.BasicAssertions ................................   Passed    0.01 sec
      Start  7: ConstTests.BasicTest
 7/37 Test  #7: ConstTests.BasicTest ......................................   Passed    0.01 sec
      Start  8: DebugTest.BasicAssertions
 8/37 Test  #8: DebugTest.BasicAssertions .................................   Passed    0.01 sec
      Start  9: LLVMFsTests.WriteFileTest
 9/37 Test  #9: LLVMFsTests.WriteFileTest .................................   Passed    0.01 sec
      Start 10: LLVMFsTests.InvalidWriteThrowTest
10/37 Test #10: LLVMFsTests.InvalidWriteThrowTest .........................   Passed    0.01 sec
      Start 11: LLVMFsTests.GetFilePathsEmpty
11/37 Test #11: LLVMFsTests.GetFilePathsEmpty .............................   Passed    0.01 sec
      Start 12: LLVMFsTests.GetFilePaths
12/37 Test #12: LLVMFsTests.GetFilePaths ..................................   Passed    0.01 sec
      Start 13: LLVMFsTests.GetFileInfo
13/37 Test #13: LLVMFsTests.GetFileInfo ...................................   Passed    0.01 sec
      Start 14: LLVMFsTests.DirectoryCreationTests
14/37 Test #14: LLVMFsTests.DirectoryCreationTests ........................   Passed    0.01 sec
      Start 15: LLVMFsTests.CreateQualifiedFilePath
15/37 Test #15: LLVMFsTests.CreateQualifiedFilePath .......................   Passed    0.01 sec
      Start 16: LLVMFsTests.CreateQualifiedFilePathExistingDirectory
16/37 Test #16: LLVMFsTests.CreateQualifiedFilePathExistingDirectory ......   Passed    0.01 sec
      Start 17: LLVMFsTests.CreateQualifiedFilePathNonExistingDirectory
17/37 Test #17: LLVMFsTests.CreateQualifiedFilePathNonExistingDirectory ...   Passed    0.01 sec
      Start 18: LLVMFsTests.CreateQualifiedFilePathExtensionChange
18/37 Test #18: LLVMFsTests.CreateQualifiedFilePathExtensionChange ........   Passed    0.01 sec
      Start 19: Imgclasstest.BasicAssertions
19/37 Test #19: Imgclasstest.BasicAssertions ..............................   Passed    0.07 sec
      Start 20: ImageProcessingTests.EnvironmentTest
20/37 Test #20: ImageProcessingTests.EnvironmentTest ......................   Passed    0.02 sec
      Start 21: ImageProcessingTests.ConvertSingleImageToTextFile
21/37 Test #21: ImageProcessingTests.ConvertSingleImageToTextFile .........   Passed    0.07 sec
      Start 22: ImageProcessingTests.ConvertSingleImageToTextFileLeak
22/37 Test #22: ImageProcessingTests.ConvertSingleImageToTextFileLeak .....   Passed    0.07 sec
      Start 23: ImageProcessingTests.WriteFileTest
23/37 Test #23: ImageProcessingTests.WriteFileTest ........................   Passed    6.05 sec
      Start 24: ImageProcessingTests.CheckForUniqueTextFile
24/37 Test #24: ImageProcessingTests.CheckForUniqueTextFile ...............   Passed    1.23 sec
      Start 25: ImageProcessingTests.BasicAssertions
25/37 Test #25: ImageProcessingTests.BasicAssertions ......................   Passed    0.27 sec
      Start 26: ImageProcessingTests.OEMvsLSTMAnalysis
26/37 Test #26: ImageProcessingTests.OEMvsLSTMAnalysis ....................   Passed    0.21 sec
      Start 27: PDFSuite.SinglePDF
27/37 Test #27: PDFSuite.SinglePDF ........................................   Passed    1.27 sec
      Start 28: SimilaritySuite.SingleString
28/37 Test #28: SimilaritySuite.SingleString ..............................   Passed    0.01 sec
      Start 29: SimilaritySuite.ImageSHA256Equal
29/37 Test #29: SimilaritySuite.ImageSHA256Equal ..........................   Passed    0.02 sec
      Start 30: SimilaritySuite.ImageSHA256Unequal
30/37 Test #30: SimilaritySuite.ImageSHA256Unequal ........................   Passed    0.02 sec
      Start 31: ParallelPerfTesseract.OEMvsLSTMAnalysisDebug
31/37 Test #31: ParallelPerfTesseract.OEMvsLSTMAnalysisDebug ..............   Passed    7.16 sec
      Start 32: PublicAPITests.GetTextFromOneImage
32/37 Test #32: PublicAPITests.GetTextFromOneImage ........................   Passed    0.08 sec
      Start 33: PublicAPITests.ProcessSimpleDir
33/37 Test #33: PublicAPITests.ProcessSimpleDir ...........................   Passed    3.33 sec
      Start 34: PublicAPITests.ProcessFilesFromDir
34/37 Test #34: PublicAPITests.ProcessFilesFromDir ........................   Passed    6.03 sec
      Start 35: PublicAPITests.Results
35/37 Test #35: PublicAPITests.Results ....................................   Passed    0.02 sec
      Start 36: PublicAPITests.AddImagesThenConvertToTextDocumentMode
36/37 Test #36: PublicAPITests.AddImagesThenConvertToTextDocumentMode .....   Passed    6.01 sec
      Start 37: PublicAPITests.AddImagesThenConvertToTextImageMode
37/37 Test #37: PublicAPITests.AddImagesThenConvertToTextImageMode ........   Passed    3.50 sec

```

<br/>

Author: [kuro337](https://github.com/kuro337)
