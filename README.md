# sve4

A Vulkan-hardware accelerated video editor.

## Building

This project uses CMake and presets.

### Prerequisites

- CMake (version 3.31+)
- A C/C++ compiler (GCC, Clang, or MSVC)
- Vulkan SDK
- libwebp

### Build Steps

1.  **Configure CMake:**

    Use the `dev` preset for local development.

    ```bash
    cmake --preset dev
    ```

2.  **Build:**

    ```bash
    cmake --build build/dev
    ```

## Testing

To run the tests, use the `dev` test preset:

```bash
ctest --preset dev
```

## Fuzzing

Currently, there is a fuzz test for `sve4_log_shorten_path`. A preset
`fuzz-common` is provided, one should override that in `CMakeUserPresets.json`.

My setting using AFL++ is the following
```jsonc
{
  "name": "fuzz",
  "inherits": [
    "fuzz-common"
  ],
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Sanitize",
    "CMAKE_C_COMPILER": "/usr/local/bin/afl-clang-fast",
    "CMAKE_CXX_COMPILER": "/usr/local/bin/afl-clang-fast++",
    // -fcf-protection=full break static libraries for some reason
    "SVE4_C_FLAGS": "-fsanitize=fuzzer -fstack-protector-strong -fstack-clash-protection -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wcast-qual -Wformat=2 -Wundef -Wshadow -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wstrict-prototypes -Wmissing-prototypes -Wimplicit-int -Wno-nullability-extension",
    "CMAKE_AR": "/usr/bin/llvm-ar",
    "CMAKE_LINKER": "/usr/bin/llvm-ld",
    "CMAKE_NM": "/usr/bin/llvm-nm",
    "CMAKE_OBJDUMP": "/usr/bin/llvm-objdump",
    "CMAKE_RANLIB": "/usr/bin/llvm-ranlib"
  }
}
```

## Project Structure

-   `libsve4_decode`: Library for decoding video frames. It seems to be using `libwebp`.
-   `libsve4_utils`: Utility library with helper functions for memory allocation and other common tasks.
-   `extern`: Contains external dependencies, like `munit`.
