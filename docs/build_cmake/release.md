# CMakeLists.txt for Static Lib + Application

```bash

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

```
