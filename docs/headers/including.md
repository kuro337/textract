# Including Headers

`src/fs.cc` and `src/fs.h`

- `.cc` must have the fn impl - (non-inline) or Visibility is Private
- `.cc` must include the Header def file `#include <fs.h>`

## Files that need Access to Funcs

- Simply add the `.cc` file to the `Add Executable` so that CMake will compile
  it along with the Files

```bash
add_executable(${TEST_EXECUTABLE} tests/${TEST_EXECUTABLE}.cc  src/fs.cc)

# Note that we can pass the .so directly for Linking deps
# ex:

target_link_libraries(target PUBLIC /usr/local/lib/libtesseract.so)

```
