#!/bin/bash

################
#  
# Build Script for All Releases
#
# Adjust the compiler and Flags appropriately if using gcc  
#
# note: this script is designed to build and release with 
#
# LLVM 
# 
# llvm version 17.0.6_1 
#
# clang++
#
# clang version 17.0.6
# Target: arm64-apple-darwin23.1.0
# Thread model: posix
#
################


opt_flags=("debug" "-O0" "release" "-O2" "safe" "-O1" "optimize" "-O3" "small" "-Os" "tiny" "-Oz" "fast" "-Ofast")

base_build_dir="./build"

mkdir -p "$base_build_dir"

echo "Starting build process..."

for ((i = 0; i < ${#opt_flags[@]}; i+=2)); do
    build_type=${opt_flags[i]}
    flag=${opt_flags[i+1]}

    echo "Building for ${build_type} with flags ${flag}..."
    build_dir="${base_build_dir}/${build_type}"
    mkdir -p "$build_dir"


    (cd "$build_dir" && cmake -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="${flag}" ../..) || { echo "CMake configuration failed for ${build_type}"; exit 1; }

    (cd "$build_dir" && make) || { echo "Make failed for ${build_type}"; exit 1; }
done

echo "Builds completed successfully."
