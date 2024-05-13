cmake_minimum_required(VERSION 3.28)
project(Textract)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(INPUT_OPEN_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images/screenshot.png")
set(IMAGE_FOLDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images")

# ─────────────────────────────────────────────────────────────────
# Macros and Optional Flags
# ─────────────────────────────────────────────────────────────────
option(DEBUG "Enable debug logging" ON)
option(DEBUG_THREADLOCAL "Enable thread-local debug logging" ON)
option(DEBUG_CACHE "Enable cache debug logging" ON)
option(DEBUG_MUTEX "Enable mutex debug logging" OFF)
option(ENABLE_TIMING "Enable timing functionality" OFF)
option(BENCHMARK "Build the benchmark tests" OFF)

# Static Linking - enforces Static Linking for all Targets
# set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# set(BUILD_SHARED_LIBS OFF)


macro(enable_options target options)
    foreach(option IN LISTS ${options})
        if(${option})
            target_compile_definitions(${target} PUBLIC ${option})
        endif()
    endforeach()
endmacro()

macro(link_llvm)  # ensure LLVM_DIR is set
  add_compile_options(-Wno-deprecated-declarations)
  set(llvm_components core support irreader)  
  llvm_map_components_to_libnames(llvm_libs ${llvm_components})
  foreach(target ${ARGN})
    target_link_libraries(${target} PUBLIC ${llvm_libs})
  endforeach()
endmacro()

macro(find_pkg PKG_NAME PKG_CONFIG_NAME)
  find_package(PkgConfig REQUIRED)
  set(PKG_VERSION_OPTIONAL ${ARGN})
  list(LENGTH PKG_VERSION_OPTIONAL PKG_VERSION_ARG_COUNT)
  if(PKG_VERSION_ARG_COUNT EQUAL 1)
    set(REQUIRED_VERSION ${PKG_VERSION_OPTIONAL})
    pkg_check_modules(${PKG_NAME} IMPORTED_TARGET ${PKG_CONFIG_NAME}>=${REQUIRED_VERSION})
    if(NOT ${PKG_NAME}_FOUND)
      message(WARNING "${PKG_NAME} version ${REQUIRED_VERSION} not found. Trying any available version.")
      pkg_check_modules(${PKG_NAME} REQUIRED IMPORTED_TARGET ${PKG_CONFIG_NAME})
    endif()
  else()
    pkg_check_modules(${PKG_NAME} REQUIRED IMPORTED_TARGET ${PKG_CONFIG_NAME})
  endif()
endmacro()


if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT APPLE)
    add_compile_definitions(_DEBUGAPP)
    add_compile_definitions("ASAN_OPTIONS=detect_leaks=1,detect_stack_use_after_return=1")
    # Enable LLVM ASan (Address Sanitizer, Mem Leaks Detection)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()

if(ENABLE_TIMING)
    add_compile_definitions(ENABLE_TIMING)
endif()

# ─────────────────────────────────────────────────────────────────
# Dependencies
# ─────────────────────────────────────────────────────────────────

find_pkg(Tesseract tesseract "5.0.0")
find_pkg(Leptonica lept)
find_package(LLVM REQUIRED CONFIG)
find_package(OpenSSL REQUIRED)
find_package(Folly CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(OpenMP)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(APPLE)
    set(TESSDATA_PREFIX "/opt/homebrew/opt/tesseract/share/tessdata" CACHE PATH "Path to Tesseract trained data")
elseif(UNIX AND NOT APPLE)
    set(TESSDATA_PREFIX "/usr/local/share/tessdata" CACHE PATH "Path to Tesseract trained data")
endif()
add_definitions(-DTESSDATA_PREFIX="${TESSDATA_PREFIX}")

# ─────────────────────────────────────────────────────────────────
# Static Library - Linking all Libraries with common_lib
# ─────────────────────────────────────────────────────────────────

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cc")
add_library(common_lib ${SRC_FILES})  

set(DEBUG_OPTIONS DEBUG DEBUG_THREADLOCAL DEBUG_CACHE DEBUG_MUTEX)
enable_options(common_lib DEBUG_OPTIONS)

target_include_directories(common_lib PUBLIC "${CMAKE_SOURCE_DIR}/include" PUBLIC ${OPENSSL_INCLUDE_DIR})

link_llvm(common_lib)

message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

target_link_libraries(common_lib
      PUBLIC GTest::gtest_main
      PUBLIC OpenSSL::Crypto
      PUBLIC OpenSSL::SSL
      PUBLIC Folly::folly
      PUBLIC PkgConfig::Tesseract
      PUBLIC PkgConfig::Leptonica)

if(OpenMP_CXX_FOUND)
  message(STATUS "Linking OpenMP with Lib")
  target_link_libraries(common_lib PUBLIC OpenMP::OpenMP_CXX)
else()
  message(STATUS "OpenMP not found")
endif()

# ─────────────────────────────────────────────────────────────────
# Executables, Tests , and Benchmarks
# ─────────────────────────────────────────────────────────────────

add_executable(main  main.cc)

target_link_libraries(main PUBLIC common_lib) 


# Add Static Binary for main.cc as well in /static

add_executable(textract_static main.cc)
target_link_libraries(textract_static PUBLIC common_lib) 
set_target_properties(textract_static PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/static")
# Ensure static linking for textract_static
set_target_properties(textract_static PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(textract_static PROPERTIES LINK_SEARCH_END_STATIC 1)


if (BENCHMARK)
    add_executable(cache_benchmark benchmarks/cache_benchmark.cc)
    target_link_libraries(cache_benchmark PUBLIC Folly::folly PUBLIC Folly::follybenchmark)
endif()

enable_testing()

set(TEST_EXECUTABLES)
file(GLOB TEST_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/tests" "tests/*.cc")

foreach(TEST_SOURCE IN LISTS TEST_SOURCES)
    get_filename_component(TEST_EXECUTABLE ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_EXECUTABLE} tests/${TEST_SOURCE})
    target_link_libraries(${TEST_EXECUTABLE} PUBLIC common_lib) # link static library with methods
    list(APPEND TEST_EXECUTABLES ${TEST_EXECUTABLE})

    target_compile_definitions(${TEST_EXECUTABLE}
        PRIVATE IMAGE_FOLDER_PATH="${IMAGE_FOLDER_PATH}"
        PRIVATE INPUT_OPEN_TEST_PATH="${INPUT_OPEN_TEST_PATH}")
    set_target_properties(${TEST_EXECUTABLE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    include(GoogleTest)
    gtest_discover_tests(${TEST_EXECUTABLE})
endforeach()


# ──────────────────────────────────────────────────────────────────────
# CodeGen -> Dynamically setting Constexprs based on Build Time Files
# ──────────────────────────────────────────────────────────────────────

set(IMAGE_DIR "${IMAGE_FOLDER_PATH}")  
file(GLOB files "${IMAGE_DIR}/*")
list(LENGTH files files_SIZE)

set(header_content "#pragma once\n#include <array>\nconstexpr std::array<const char*, ${files_SIZE}> fileNames = {\n")
foreach(file IN LISTS files)
    get_filename_component(fname "${file}" NAME)
    set(header_content "${header_content}    \"${IMAGE_DIR}/${fname}\",\n")  
endforeach()

set(header_content "${header_content}};\n") 
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")  
file(WRITE "${CMAKE_BINARY_DIR}/include/generated_files.h" "${header_content}")
include_directories(${CMAKE_BINARY_DIR}/include)  


message(STATUS "Codegen Generated fileNames array<const char*> : #include <generated_files.h>")

message(STATUS "Build Successful.\nInvoke all tests:\n ctest")
