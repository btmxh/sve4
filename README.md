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

## Project Structure

-   `libsve4_decode`: Library for decoding video frames. It seems to be using `libwebp`.
-   `libsve4_utils`: Utility library with helper functions for memory allocation and other common tasks.
-   `extern`: Contains external dependencies, like `munit`.
