cmake_minimum_required(VERSION 3.20)
project(opencvOCR)

# change standard as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenMP) 
find_package(Folly CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(CURL REQUIRED)

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
  PUBLIC ${OpenCV_LIBS}
  PUBLIC ${CURL_LIBRARIES}
)

if(OpenMP_CXX_FOUND)
    message(STATUS "Using OpenMP")
    target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
else()
    message(STATUS "OpenMP not found")

endif()

# tests


enable_testing()

add_executable(ocr_test         tests/ocr_test.cc)
add_executable(atomic_test      tests/atomic_test.cc)
add_executable(similarity_test  tests/similarity_test.cc)
add_executable(threadlocal_test tests/threadlocal_test.cc)
add_executable(cache_benchmark  benchmarks/cache_benchmark.cc)

target_link_libraries(
  cache_benchmark
  PUBLIC GTest::gtest_main
  PUBLIC ${CURL_LIBRARIES}
  PUBLIC ${OpenCV_LIBS}
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
  PUBLIC Folly::folly
  PUBLIC Folly::follybenchmark
)


target_link_libraries(
  ocr_test
  PUBLIC GTest::gtest_main
  PUBLIC ${CURL_LIBRARIES}
  PUBLIC ${OpenCV_LIBS}
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
  PUBLIC Folly::folly
)

target_link_libraries(
  threadlocal_test
  PUBLIC GTest::gtest_main
  PUBLIC ${CURL_LIBRARIES}
  PUBLIC ${OpenCV_LIBS}
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
  PUBLIC Folly::folly
)


target_link_libraries(
  similarity_test
  PUBLIC GTest::gtest_main
  PUBLIC ${CURL_LIBRARIES}
  PUBLIC ${OpenCV_LIBS}
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
  PUBLIC Folly::folly
)

target_link_libraries(
  atomic_test 
  PUBLIC GTest::gtest_main
  PUBLIC ${CURL_LIBRARIES}
  PUBLIC ${OpenCV_LIBS}
  PUBLIC OpenSSL::Crypto
  PUBLIC tesseract
  PUBLIC leptonica
  PUBLIC Folly::folly
)


include(GoogleTest)
gtest_discover_tests(ocr_test)
gtest_discover_tests(similarity_test)
gtest_discover_tests(atomic_test)


if(OpenMP_CXX_FOUND)
    message(STATUS "Using OpenMP")
    target_link_libraries(ocr_test PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(similarity_test PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(similarity_test PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(threadlocal_test PUBLIC OpenMP::OpenMP_CXX)
else()
    message(STATUS "OpenMP not found")

endif()

