```bash
# Static Library : 1. Define src folder, add lib, set Include Dir for Static Lib
include_directories(${CMAKE_SOURCE_DIR}/include)  # Make sure the include Headers are included
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cc")
add_library(common_lib ${SRC_FILES})  # Creates a static library by default
target_include_directories(common_lib PUBLIC "${CMAKE_SOURCE_DIR}/include")

# with each executable

target_link_libraries(${TEST_EXECUTABLE} common_lib) # link static library with methods

# making sure all .cc files from src are included

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cc")


```
