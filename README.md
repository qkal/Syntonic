# Syntonic

AppKit for C: a macOS library that wraps AppKit class by class behind pure C23
headers, so a Mac app can be written in C and still look and behave like one
written in Swift.

## Status

Early. The build, test and bundle tooling is in place; the wrappers are not.

## Requirements

The Command Line Tools, CMake and `just`. Xcode.app is not needed. The
deployment floor is macOS 26.

## Getting started

```sh
just build        # configure and build the library and its tests
just test         # run the CTest suites
just bundle build/test_smoke Smoke dev.kaino.syntonic.smoke
```

`just --list` shows every recipe, including the sanitizer, leak and header
checks.

## Consuming the library

Syntonic is consumed as source:

```cmake
add_subdirectory(third_party/syntonic)
target_link_libraries(my_app PRIVATE syntonic)
```

```c
#include <syntonic/syntonic.h>
```

## License

MIT. See [LICENSE](LICENSE).
