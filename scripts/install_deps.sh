#!/bin/bash

# build from source
# folly
# googletest
# cmake

sudo apt-get update
sudo apt-get install libssl-dev
sudo apt-get install libleptonica-dev
sudo apt-get install libtesseract-dev
sudo apt-get install tesseract-ocr-eng  

echo "All dependencies installed!"

# cmake -DCMAKE_CXX_COMPILER=/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/home/linuxbrew/.linuxbrew/opt/llvm/bin/clang ../../