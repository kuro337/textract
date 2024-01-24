cmake_minimum_required(VERSION 3.28)
project(opencvOCR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions("ASAN_OPTIONS=detect_leaks=1,detect_stack_use_after_return=1")
    add_compile_definitions(_DEBUGAPP)
    # Enable LLVM ASan (Address Sanitizer, Mem Leaks Detection)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()

find_package(OpenSSL REQUIRED)
find_package(OpenMP) 
find_package(Folly CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(CURL REQUIRED)

find_package(OpenCV)
if(OpenCV_FOUND)
    add_definitions(-DUSE_OPENCV)
endif()

include_directories("/opt/homebrew/opt/tesseract/include") 
link_directories("/opt/homebrew/opt/tesseract/lib")
include_directories("/opt/homebrew/opt/leptonica/include") 
link_directories("/opt/homebrew/opt/leptonica/lib")

# gtest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE  
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # windows setting
FetchContent_MakeAvailable(googletest)


# main 

add_executable(
 main 
 main.cc
)

target_link_libraries(main
  PUBLIC GTest::gtest_main
  PUBLIC Folly::folly
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
 # PUBLIC ${OpenCV_LIBS}
  PUBLIC ${CURL_LIBRARIES}
)

if(OpenMP_CXX_FOUND)
    message(STATUS "Using OpenMP")
    target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
else()
    message(STATUS "OpenMP not found")

endif()

# tests

add_executable(cache_benchmark  benchmarks/cache_benchmark.cc)

target_link_libraries(
  cache_benchmark
  PUBLIC GTest::gtest_main
  PUBLIC ${CURL_LIBRARIES}
# PUBLIC ${OpenCV_LIBS}
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
  PUBLIC Folly::folly
  PUBLIC Folly::follybenchmark
)



enable_testing()

set(TEST_EXECUTABLES tesseractparallel_test bulk_test pdf_test textractapi_tests ocr_test atomic_test similarity_test threadlocal_test)

foreach(TEST_EXECUTABLE IN LISTS TEST_EXECUTABLES)
    add_executable(${TEST_EXECUTABLE} tests/${TEST_EXECUTABLE}.cc)
    set_target_properties(${TEST_EXECUTABLE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    # Common libraries
    target_link_libraries(${TEST_EXECUTABLE} 
      PUBLIC GTest::gtest_main
      PUBLIC ${CURL_LIBRARIES}
      # PUBLIC ${OpenCV_LIBS}
      PUBLIC OpenSSL::Crypto
      PUBLIC tesseract
      PUBLIC leptonica
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

