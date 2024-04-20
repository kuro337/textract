# Declaring Constants during Compilation

```bash

set(IMAGE_FOLDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images")
set(INPUT_OPEN_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images/screenshot.png")


  target_compile_definitions(${TEST_EXECUTABLE}
        PRIVATE IMAGE_FOLDER_PATH="${IMAGE_FOLDER_PATH}"
        PRIVATE INPUT_OPEN_TEST_PATH="${INPUT_OPEN_TEST_PATH}"
    )

const auto *const imgFolder = IMAGE_FOLDER_PATH;

const std::string imgFolder = IMAGE_FOLDER_PATH;

```

```cpp
namespace {
    const auto *const inputOpenTest = INPUT_OPEN_TEST_PATH;

    const std::string cmake_var = std::string(VAR) + "/somefile.txt";

} // namespace
```
