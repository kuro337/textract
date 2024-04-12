#!/bin/bash

################
#  
# Build Script for Releases
#
# - build all releases
# 
# ./build_all.sh 
#
# - build only one release
#
# ./build_all.sh release
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

#!/bin/bash

opt_flags=("debug" "-O0" "release" "-O2" "safe" "-O1" "optimize" "-O3" "small" "-Os" "tiny" "-Oz" "fast" "-Ofast")

base_build_dir="./build"


mkdir -p "$base_build_dir"

requested_build_type=$1
run_tests=$2

echo "Starting build process..."

for ((i = 0; i < ${#opt_flags[@]}; i+=2)); do
    build_type=${opt_flags[i]}
    flag=${opt_flags[i+1]}


    if [ -z "$requested_build_type" ] || [ "$requested_build_type" == "$build_type" ]; then
        echo "Building for ${build_type} with flags ${flag}..."
        build_dir="${base_build_dir}/${build_type}"
        mkdir -p "$build_dir"

        extra_cmake_flags=""
        cmake_build_type="Release"
        if [ "$build_type" == "debug" ]; then
            extra_cmake_flags="-D_DEBUGAPP=ON"
            cmake_build_type="Debug"
        fi

        echo "----------------------------------------"
        echo "Running CMake command for ${build_type} build:"
        echo "-DCMAKE_BUILD_TYPE=${cmake_build_type} -DCMAKE_CXX_FLAGS=${flag} -DENABLE_TIMING=ON ${extra_cmake_flags} ../.."
        echo "----------------------------------------"
        # -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang 
        (cd "$build_dir" && cmake  -DCMAKE_BUILD_TYPE=${cmake_build_type} -DENABLE_TIMING=ON -DCMAKE_CXX_FLAGS="${flag}" ${extra_cmake_flags} ../..) || { echo "CMake configuration failed for ${build_type}"; exit 1; }
        (cd "$build_dir" && make) || { echo "Make failed for ${build_type}"; exit 1; }

        echo -e "Running Tests in tests directory\n"
        if [ "$run_tests" == "--test" ]; then
            echo "Running tests for ${build_type}..."
            test_dir="${build_dir}/tests"
            test_executables=()
            for test_executable in "$test_dir"/*; do
                if [ -x "$test_executable" ] && [ -f "$test_executable" ] && [[ "$test_executable" == *"test"* ]]; then
                    test_executables+=("$test_executable")
                fi
            done
            
            for test_executable in "${test_executables[@]}"; do
                echo "Running test: $test_executable"
                (
                cd "$(dirname "$test_executable")"
                ./$(basename "$test_executable")
                ) || { echo "Test failed: $test_executable"; exit 1; }
            done
        fi
    fi
done

cp build/debug/compile_commands.json build/

echo "Builds completed successfully."





