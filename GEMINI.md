This is a Vulkan-hardware accelerated video editor project named `sve4`.

It is built with C/C++ and CMake.

The project is structured into two main libraries:
- `libsve4_decode`: For decoding video frames.
- `libsve4_utils`: For common utilities.

It uses `munit` for testing and `libwebp` for image decoding.

The user wants to use presets for local development and prefers to use clang.

Here are the user presets, located in `CMakeUserPresets.json`:

```json
{
  "version": 10,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 31,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "dev",
      "inherits": [
        "dev-mode",
        "ci-linux"
      ],
      "binaryDir": "${sourceDir}/build/dev/",
      "cacheVariables": {
        "BUILD_MCSS_DOCS": "ON",
        "CMAKE_C_COMPILER": "clang",
        "CMAKE_CXX_COMPILER": "clang++"
      }
    },
    {
      "name": "dev-coverage",
      "binaryDir": "${sourceDir}/build/coverage",
      "inherits": [
        "dev-mode",
        "clang-tidy",
        "ci-linux",
        "coverage-linux"
      ]
    }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "noTestsAction": "error"
      }
    }
  ]
}
```
