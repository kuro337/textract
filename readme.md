<br/>
<br/>

<div align="center">
  <img alt="textract logo"  src="assets/logo.png">
</div>

<br/>
<br/>

# textract

<br/>

_Single Header High Performance_ **C++ Image Processing** Library to read content from Images and transform Images to text files.

<br/>

<hr/>

<br/>

Build from Source using **CMake**

#### Dependencies

<br/>

```bash
brew install opencv openssl libomp folly tesseract
```

<br/>

#### Build

<br/>

```bash

cd textract && mkdir build && cd build
cmake ..
make

# using LLVM and Clang++ directly
cmake -DCMAKE_CXX_COMPILER=/path/to/clang++ -DCMAKE_C_COMPILER=/path/to/clang ..
make

# getting clang++ and clang paths
echo $(brew --prefix llvm)/bin/clang++
echo $(brew --prefix llvm)/bin/clang

```

<br/>

## Design

<br/>

#### OpenCV and Tesseract

For Processing images and using _Tesseract OCR_ to extract text from Images.

<hr/>

#### OpenSSL

For generating _SHA256_ hashes from Image bytes and metadata.

<hr/>

#### OpenMP

To provide _parallelization_ on systems for processing.

<hr/>

#### Folly

_textract_ uses Folly's **AtomicUnorderedInsertMap** for the Cache implementation to provide wait free parallel access to the Cache

[Folly](https://github.com/facebook/folly)::[AtomicUnorderedInsertMap](https://github.com/facebook/folly/blob/main/folly/AtomicUnorderedMap.h)

<hr>

<br/>

## Usage

<br/>

Process Images and get their textual content

<br/>

```cpp
#include "imgtotext.h"

int main() {
    imgstr::ImageTranslator app = imgstr::ImageTranslator();

    std::vector<std::string> results = app.processImages("cs101_notes.png","bio.jpeg");

    app.writeImageTextOut("cs101_notes.png", "cs_notes.txt");

    return 0;
}

```

<br/>

Process all valid Image files from a directory and create text files

<br/>

```cpp
#include "imgtotext.h"

int main() {

    /* Process 10000 images using parallelism */

    imgstr::ImageTranslator app = imgstr::ImageTranslator(10000);


    app.processImagesWriteResults("/path/to/dir");


    return 0;
}

```

<hr>

<br/>

### In Memory Cache Benchmarks

<br/>

```bash
============================================================================
/textract/benchmarks/cache_benchmark.cc     relative  time/iter   iters/s
============================================================================
UnorderedMapMutexedSingleThreaded                         254.95ns     3.92M
UnorderedMapMutexedMultiThreaded                            3.23us   309.19K
UnorderedMapMutexedMaxThreads                               7.28us   137.27K
ConcurrentHashMapSingleThreaded                           859.52ns     1.16M
ConcurrentHashMapMultiThreaded                              3.41us   293.37K
ConcurrentHashMapMaxThreads                                26.40us    37.87K
ConcurrentHashMapComplexSingleThreaded                      1.43us   700.82K
ConcurrentHashMapComplexMultiThreaded                       4.81us   207.69K
ConcurrentHashMapComplexMaxThreads                         32.92us    30.38K
AtomicUnorderedMapSingleThreaded                          159.61ns     6.27M
AtomicUnorderedMapMultiThreaded                           403.47ns     2.48M
AtomicUnorderedMapMaxThreads                                1.63us   611.78K
AtomicUnorderedMapComplexSingleThreaded                   917.79ns     1.09M
AtomicUnorderedMapComplexMultiThreaded                      2.44us   409.85K
AtomicUnorderedMapComplexMaxThreads                        12.62us    79.25K
============================================================================

```

<br/>

It can be seen **AtomicUnorderedInsertMap** is over **8x** faster than the Concurrent HashMap.

[AtomicUnorderedInsertMap](https://github.com/facebook/folly/blob/main/folly/AtomicUnorderedMap.h) provides an overview of the tradeoffs.

<hr>

<br/>

Author: [kuro337](https://github.com/kuro337)
