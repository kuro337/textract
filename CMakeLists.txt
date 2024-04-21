cmake_minimum_required(VERSION 3.28)
project(opencvOCR)

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

# recommended for include is to have headers in an include folder
include_directories(${CMAKE_SOURCE_DIR}/src) # can access any headers 
include_directories(${CMAKE_SOURCE_DIR})
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

add_executable(
 main
 main.cc
)

target_link_libraries(main
  PUBLIC GTest::gtest_main
  PUBLIC Folly::folly
  PUBLIC OpenSSL::Crypto
  PUBLIC PkgConfig::Tesseract 
  PUBLIC PkgConfig::Leptonica
)

if(OpenMP_CXX_FOUND)
    message(STATUS "Using OpenMP")
    target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
else()
    message(STATUS "OpenMP not found")
endif()


if (BENCHMARK)
    add_executable(cache_benchmark benchmarks/cache_benchmark.cc)
    target_link_libraries(
      cache_benchmark
      PUBLIC GTest::gtest_main
      PUBLIC OpenSSL::Crypto
      PUBLIC PkgConfig::Tesseract 
      PUBLIC PkgConfig::Leptonica 
      PUBLIC Folly::folly
      PUBLIC Folly::follybenchmark
    )
endif()

enable_testing()

set(TEST_EXECUTABLES fs_test ocr_test imgclass_test
                     tesseractparallel_test bulk_test pdf_test 
                     textractapi_tests  atomic_test similarity_test 
                     threadlocal_test)

foreach(TEST_EXECUTABLE IN LISTS TEST_EXECUTABLES)
    add_executable(${TEST_EXECUTABLE} tests/${TEST_EXECUTABLE}.cc  src/fs.cc)

    target_compile_definitions(${TEST_EXECUTABLE}
    PRIVATE IMAGE_FOLDER_PATH="${IMAGE_FOLDER_PATH}"
    PRIVATE INPUT_OPEN_TEST_PATH="${INPUT_OPEN_TEST_PATH}"
    )
    
    set_target_properties(${TEST_EXECUTABLE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    # Common libraries
    target_link_libraries(${TEST_EXECUTABLE}
      PUBLIC GTest::gtest_main
      PUBLIC OpenSSL::Crypto
      PUBLIC OpenSSL::SSL
      PUBLIC PkgConfig::Tesseract 
      PUBLIC PkgConfig::Leptonica 
      PUBLIC Folly::folly
    )

    if(OpenMP_CXX_FOUND)
      message(STATUS "Using OpenMP for ${TEST_EXECUTABLE}")
      target_link_libraries(${TEST_EXECUTABLE} PUBLIC OpenMP::OpenMP_CXX)
    else()
      message(STATUS "OpenMP not found for ${TEST_EXECUTABLE}")
    endif()
    include(GoogleTest)
    gtest_discover_tests(${TEST_EXECUTABLE})

endforeach()


add_llvm_support(main ${TEST_EXECUTABLES})

