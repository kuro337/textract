#!/bin/bash

brew update
brew upgrade

brew install openssl
brew install libomp 
brew install gflags
brew install curl
brew install tesseract
brew install leptonica
brew install folly
brew install googletest

if ! command -v cmake &> /dev/null; then
    brew install cmake
fi

echo "All dependencies installed!"
