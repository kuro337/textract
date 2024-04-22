# Code Generation for True Compile Time Computation

If we know that during Compile Time - we want to compute a List of Static Files

We can - write a Header file that includes those Constants during the Build
Process

1. Set the Base Folder relative to `CMakeLists.txt`
2. Get all File Names + Count of Files - `files` , `count`
3. Write a Header file such as `codegen_consts.h` containing Data

```cpp
#pragma once
#include <array>
constexpr std::array<const char*, 8> fileNames = {
    "/image/imgapp/images/4ebfa52f6d06a18c.jpg",
    "/image/imgapp/images/z0d2041bcb10cb63a.jpg",
};
```

4. Include the Folder where we write our Header
   `include_directories(${CMAKE_BINARY_DIR}/include)`

5. Then in our code we can `#include codegen_consts.h` and use the `constexpr`
   vars

```bash
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

```
