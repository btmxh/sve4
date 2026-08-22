# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`sve4` — a Vulkan-hardware accelerated video editor, written in C17 (CMake declares C and CXX, but all source is C). Currently three static/shared libraries; there is no application executable yet.

## Build & test

Dependencies live in `extern/` as git submodules (`munit`, `tinycthread`, `arena`, `civetweb`) — clone/pull with `--recurse-submodules`. System dependencies (Vulkan+volk, libwebp, FFmpeg, GLFW) are all found `QUIET` and are **optional**; missing ones silently disable code paths and tests (see "Optional-dependency pattern").

A Nix flake provides the full dev environment (`nix develop`, or `direnv allow` — `.envrc` is already set up). It exports `CMAKE_EXPORT_COMPILE_COMMANDS=ON` and the `LD_LIBRARY_PATH`/`VK_LAYER_PATH` that volk's `dlopen()` of the Vulkan loader needs on NixOS.

```bash
cmake --preset dev            # configure into build/dev
cmake --build build/dev       # build
ctest --preset dev            # run all tests
ctest --preset dev -R test_log_tty            # run one test by name
ctest --preset dev -R test_decode_ -V         # one library's tests, verbose
```

Presets: `CMakePresets.json` holds the hidden/CI presets; `dev` and `dev-coverage` come from `CMakeUserPresets.json` (gitignored — it exists locally and uses clang). CI presets are `ci-ubuntu`, `ci-macos`, `ci-windows`, `ci-coverage`, `ci-sanitize`, plus `fuzz-common` for fuzzing.

Other targets: `cmake --build build/coverage -t coverage` (lcov + genhtml, needs `ENABLE_COVERAGE=ON`), `cmake --build build/dev -t docs` (Doxygen + m.css, needs `BUILD_MCSS_DOCS=ON`).

clang-tidy runs as part of compilation only when `SVE4_CLANG_TIDY=ON` (the `clang-tidy` preset, used by `ci-ubuntu` and `dev-coverage`) and is `--warnings-as-errors=*` there. The `dev` preset does not run it, so a clean `dev` build can still fail CI.

Formatting/linting is enforced by pre-commit (`pre-commit run --all-files`): clang-format, gersemi + cmakelint for CMake files, codespell, mbake for Makefiles.

## Architecture

Dependency order: `sve4::utils` ← `sve4::log` ← `sve4::decode`. Each directory `libsve4_X/` builds target `sve4_X` with alias `sve4::X`; includes are rooted at the repo root, so headers are included as `"libsve4_utils/buffer.h"`.

**libsve4_utils** — the primitives everything else is built on:
- `allocator.h`: a vtable struct `sve4_allocator_t` (alloc/calloc/grow/free + a two-pointer `state`) passed as an optional `_Nullable` parameter throughout the codebase; `NULL` means the libc default. `arena.h` wraps tsoding/arena behind that interface.
- `buffer.h`: `sve4_buffer_ref_t` — atomically refcounted, allocator-aware buffer with a flexible array member and an optional destructor. This is the ownership primitive: decoders, frames, log user-data and demuxers are all held as buffer refs, so "owning" vs "not owning" is expressed by whether a ref was taken.
- `formats.h`: tagged formats (`{source, int32 value}`) so an FFmpeg pixfmt/samplefmt can travel through the API without every consumer depending on FFmpeg.
- `error.h`: `sve4_error_trace_t` (`__FILE__`/`__LINE__`), embedded in error structs only when `NDEBUG` is undefined.

**libsve4_log** — thread-safe logging with a registry of configs. `api.h` is what callers use (`sve4_log_info(...)` etc., plus `sve4_flog`/`sve4_glog` for explicit log ids). `init.h` is the configuration side: `sve4_log_init()`, then `sve4_log_add_config()` registers a `sve4_log_config_t` (level + callback + path-shortening + id→name mapping); several may be active at once. Each `sve4_log_id_t` names a source (application, sve4_decode, sve4_log, ffmpeg, vulkan, glfw). A translation unit's default id is set at build time via `-DSVE4_LOG_ID_MAIN=...` in its CMakeLists. `ffmpeg.c`, `vulkan.c`, `glfw.c` bridge those libraries' callbacks into this system. `init_test.h`/`init_test.c` are compiled into the library when `BUILD_TESTING` is on and route logs into munit.

**libsve4_decode** — `decoder.h` is the backend-agnostic front: `sve4_decode_decoder_open()` takes a `sve4_decode_decoder_config_t` (url, backend, allocators, stream chooser, plus per-FFmpeg-call option structs) and fills a `sve4_decode_decoder_t` whose `get_frame`/`seek` are function pointers into the chosen backend. `sve4_decode_select_backend()` picks libwebp or FFmpeg when `BACKEND_AUTO`.
- FFmpeg path: `ffmpeg_demuxer` owns the `AVFormatContext` and a linked list of decoders sharing it; `ffmpeg_demuxer_thread` optionally runs demuxing on a tinycthread thread; `ffmpeg_packet_queue` is a mutex+condvar `AVFifo` of packets per decoder, with deadline-based push/pop. Multiple decoders can share one demuxer (`config.demuxer`), which is why packets are queued per-decoder rather than pulled directly.
- Frames: `sve4_decode_frame_t` is a tagged handle (`RAM_FRAME`, `VULKAN`, `AVFRAME`) over a buffer ref. `ram_frame.h` is the CPU-side planar layout; `vulkan_frame.c/h` are currently empty placeholders for the GPU path.
- `read.h`/`read.c`: URL reader used for the non-FFmpeg backends.

Errors are per-library value types, not codes: `sve4_decode_error_t` is `{source, error_code}` (+ trace in debug), constructed via `sve4_decode_defaulterr(...)` / `sve4_decode_ffmpegerr(...)` and tested with `sve4_decode_error_is_success()`. Keep that shape when adding error paths.

## Conventions

- All pointers carry clang nullability qualifiers (`_Nonnull` / `_Nullable`); `libsve4_utils/defines.h` defines them away on non-clang compilers. New pointer parameters and struct members are expected to be annotated.
- Public symbols are exported with the generated `SVE4_<LIB>_EXPORT` macros from `sve4_<lib>_export.h` (produced by `sve4_generate_export_header`); shared-library builds are part of CI, so an unexported public function will fail there.
- Internal helpers are prefixed `sve4__` (double underscore); the user-facing macro wrapping them is `sve4_`.
- Every target must go through `sve4_set_target_default_properties(TARGETS ...)` — it applies C17, `SVE4_C_FLAGS` (a long, strict warning set including `-Wconversion`/`-Wsign-conversion`), clang-tidy, `-DSVE4_ROOT_DIR=...` and the root include dir.
- Optional-dependency pattern: guard the source files in `CMakeLists.txt` with `if(<Dep>_FOUND)`, add a `PUBLIC` compile definition (`SVE4_DECODE_HAVE_WEBP`, `SVE4_LOG_HAVE_VULKAN`, `SVE4_UTILS_HAVE_FFMPEG`, …), and guard the headers/tests with the same macro. Follow it for any new optional feature.
- Include order is enforced by `.clang-format`'s `IncludeCategories`: stdlib `<...>` / export header / `"libsve4_*"` / other `<...>` / relative `"*.h"`.

## Tests

munit-based, one executable per source file, registered via the `sve4_add_test(PREFIX <lib> SOURCE <file.c> LIBRARIES ...)` helper in the root `CMakeLists.txt`; the test name becomes `test_<prefix>_<source>`. Tests are skipped entirely when `SVE4_ENABLE_FUZZING` is on. munit is built with `MUNIT_NO_FORK`, so a crashing test takes the whole executable down.

Test data lives in `assets/` and is referenced as `#define ASSETS_DIR "../../../../assets/"` — a path relative to the test's build directory, which assumes the binary dir is exactly two levels deep (`build/<name>/`). Keep that shape when adding presets. `assets/generated/` is produced by its own Makefile (ImageMagick + the libwebp tools `cwebp`/`img2webp`/`webpmux`, plus ffmpeg) and is checked in; `assets/Makefile` regenerates the CRLF fixture.

`libsve4_log/fuzz/shorten_path.c` is the only fuzz target (`sve4_log_shorten_path`); see README for the AFL++ user preset.
