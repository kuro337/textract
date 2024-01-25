# ASan

llvm includes ASan for address sanitization and memory leaks

<br/>

program that leaks memory

```cpp

#include <iostream>

int main() {

  // Allocate memory for an integer, but never deallocate it
  int *leaked_memory = new int(42);

  std::cout << "This program leaks memory!" << std::endl;
  return 0;
}

/*

Compile  | -g adds debug info

clang++ -fsanitize=address -g memleak.cc

Run

ASAN_OPTIONS=detect_leaks=1 ./a.out


*/

```
