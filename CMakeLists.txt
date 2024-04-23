cmake_minimum_required(VERSION 3.28)
project(opencvOCR)

# src/*.cc      : Concrete Impls of Methods declared in Header Files
# include/*.h   : Header Files defining Shared Methods
# tests/*.cc    : Tests for Application
# textract.h    : Main Header File
# build/include : Codegen generated Header Files for Compile Time Dynamic Constexpressions

# include_directories  
# Adds Dirs for all Targets after the call

# target_include_directories (modern)
# Sets Include Path for a Specific Target and its dependents

# ex: common_lib -> target_include_directories(src/ and include/)
# include/ has fs.h which provides methods
# Link common_lib with test_exe -> test_exe can #include <fs.h> without including it during build
# normally this would happen by include_directories(${CMAKE_SOURCE_DIR}/include)  

# Private Include Convention : #include "fs.h"
# Public Include  Convention : #include <fs.h>

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(IMAGE_FOLDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images")
set(INPUT_OPEN_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images/screenshot.png")

macro(add_llvm_support)  # ensure LLVM_DIR is set
  add_compile_options(-Wno-deprecated-declarations)
  set(llvm_components core support irreader)  
  llvm_map_components_to_libnames(llvm_libs ${llvm_components})
  foreach(target ${ARGN})
    target_link_libraries(${target} PUBLIC ${llvm_libs})
  endforeach()
endmacro()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ENABLE_TIMING "Enable timing functionality" OFF)
option(BENCHMARK "Build the benchmark tests" OFF)


if(ENABLE_TIMING)
    add_compile_definitions(ENABLE_TIMING)
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT APPLE)
    add_compile_definitions(_DEBUGAPP)
    add_compile_definitions("ASAN_OPTIONS=detect_leaks=1,detect_stack_use_after_return=1")
    # Enable LLVM ASan (Address Sanitizer, Mem Leaks Detection)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()


#--------------------------------------------------------------------
# DEPENDENCIES
#--------------------------------------------------------------------

find_package(LLVM REQUIRED CONFIG)
find_package(OpenSSL REQUIRED)
find_package(OpenMP)
find_package(Folly CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)


find_package(Tesseract QUIET)
if (NOT Tesseract_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Tesseract REQUIRED IMPORTED_TARGET tesseract)
endif()

find_package(Leptonica QUIET)
if (NOT Leptonica_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Leptonica REQUIRED IMPORTED_TARGET lept)
endif()

# gtest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # windows setting
FetchContent_MakeAvailable(googletest)

#--------------------------------------------------------------------
# STATIC LIBRARY
#--------------------------------------------------------------------

# Static Library : 1. Define src folder, add lib, set Include Dir for Static Lib

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cc")
add_library(common_lib ${SRC_FILES})  # Creates a static library by default

# CURR_ Linking is Transitive: so any targets linking to common lib dont need to be Linked to these libs
target_link_libraries(common_lib
      PUBLIC GTest::gtest_main
      PUBLIC OpenSSL::Crypto
      PUBLIC OpenSSL::SSL
      PUBLIC PkgConfig::Tesseract
      PUBLIC PkgConfig::Leptonica
      PUBLIC Folly::folly)

add_llvm_support(common_lib)

if(OpenMP_CXX_FOUND)
  message(STATUS "Linking OpenMP with Lib")
  target_link_libraries(common_lib PUBLIC OpenMP::OpenMP_CXX)
else()
  message(STATUS "OpenMP not found")
endif()


# Any target linking with common_lib has access to all libs common_lib is linked with
target_include_directories(common_lib PUBLIC "${CMAKE_SOURCE_DIR}/include")

# link main to common_lib to have access to all headers and libs
add_executable(main  main.cc)
target_link_libraries(main PUBLIC common_lib) # link with common_lib


if (BENCHMARK)
    add_executable(cache_benchmark benchmarks/cache_benchmark.cc)
    target_link_libraries(
      cache_benchmark PUBLIC Folly::folly PUBLIC Folly::follybenchmark
    )
endif()

enable_testing()

set(TEST_EXECUTABLES)

file(GLOB TEST_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/tests" "tests/*.cc")


foreach(TEST_SOURCE IN LISTS TEST_SOURCES)
    get_filename_component(TEST_EXECUTABLE ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_EXECUTABLE} tests/${TEST_SOURCE})
    target_link_libraries(${TEST_EXECUTABLE} PUBLIC common_lib) # link static library with methods

    list(APPEND TEST_EXECUTABLES ${TEST_EXECUTABLE}) # add to TEST_EXE for Linking llvm

    target_compile_definitions(${TEST_EXECUTABLE}
        PRIVATE IMAGE_FOLDER_PATH="${IMAGE_FOLDER_PATH}"
        PRIVATE INPUT_OPEN_TEST_PATH="${INPUT_OPEN_TEST_PATH}"
    )

    set_target_properties(${TEST_EXECUTABLE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    include(GoogleTest)
    gtest_discover_tests(${TEST_EXECUTABLE})
endforeach()

# add_llvm_support(main ${TEST_EXECUTABLES})


#### Prepopulating files during Compile Time
# set(IMAGE_FOLDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images") rel to CMakeLists.txt
set(IMAGE_DIR "${IMAGE_FOLDER_PATH}")  

# 1. Get all Files + Calculate Total Size
file(GLOB files "${IMAGE_DIR}/*")
list(LENGTH files files_SIZE)

# 2. CodeGen -> Start writing the header file content
set(header_content "#pragma once\n#include <array>\nconstexpr std::array<const char*, ${files_SIZE}> fileNames = {\n")

foreach(file IN LISTS files)  # 3. Add each file name to the header content
    get_filename_component(fname "${file}" NAME)
    # Set folder path + image file name as Str in Arr
    set(header_content "${header_content}    \"${IMAGE_DIR}/${fname}\",\n")  
endforeach()

set(header_content "${header_content}};\n") # Add final '};' to end of Arr
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")  # mkdir -p && write Codegen to a Header File there
file(WRITE "${CMAKE_BINARY_DIR}/include/generated_files.h" "${header_content}")

include_directories(${CMAKE_BINARY_DIR}/include) # make codegen available

### Usage: #include <generated_files.h> ###

