#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "defines.h"

typedef enum {
  SVE4_LOG_LEVEL_DEFAULT = -1,
  SVE4_LOG_LEVEL_DEBUG,
  SVE4_LOG_LEVEL_INFO,
  SVE4_LOG_LEVEL_WARNING,
  SVE4_LOG_LEVEL_ERROR,
  SVE4_LOG_LEVEL_MAX,
} sve4_log_level_t;

typedef enum {
  SVE4_LOG_ID_APPLICATION,
  SVE4_LOG_ID_DEFAULT_SVE4_DECODE,
  SVE4_LOG_ID_DEFAULT_SVE4_LOG,
  SVE4_LOG_ID_DEFAULT_FFMPEG,
  SVE4_LOG_ID_DEFAULT_VULKAN,
} sve4_log_id_t;

#ifndef SVE4_LOG_ID_MAIN
#define SVE4_LOG_ID_MAIN SVE4_LOG_ID_APPLICATION
#endif

// freestanding logging API, use the macros if possible
void sve4__flog(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                sve4_log_level_t level, const char* _Nonnull fmt, ...)
    sve4_gnu_attribute((__format__(printf, 5, 6)));
void sve4__flogv(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                 sve4_log_level_t level, const char* _Nonnull fmt, va_list args)
    sve4_gnu_attribute((__format__(printf, 5, 0)));
// flog and then exit with status code 1
void sve4__panic(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                 const char* _Nonnull fmt, ...)
    sve4_gnu_attribute((__noreturn__)) sve4_gnu_attribute((__cold__))
        sve4_gnu_attribute((__format__(printf, 4, 5)));

#define sve4_flog(log_id, ...)                                                 \
  sve4__flog(log_id, __FILE__, __LINE__, __VA_ARGS__)
#define sve4_panic(...)                                                        \
  sve4__panic(__FILE__, __LINE__, sve4_log_id_application, __VA_ARGS__)

// generic logging API
void sve4_glog(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
               bool endl, sve4_log_level_t level, const char* _Nonnull fmt, ...)
    sve4_gnu_attribute((__format__(printf, 6, 7)));
void sve4_glogv(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                bool endl, sve4_log_level_t level, const char* _Nonnull fmt,
                va_list args) sve4_gnu_attribute((__format__(printf, 6, 0)));

#define sve4_log(...)                                                          \
  sve4_glog(SVE4_LOG_ID_MAIN, __FILE__, __LINE__, false, __VA_ARGS__)
#define sve4_log_debug(...) sve4_log(SVE4_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define sve4_log_info(...) sve4_log(SVE4_LOG_LEVEL_INFO, __VA_ARGS__)
#define sve4_log_warn(...) sve4_log(SVE4_LOG_LEVEL_WARNING, __VA_ARGS__)
#define sve4_log_error(...) sve4_log(SVE4_LOG_LEVEL_ERROR, __VA_ARGS__)

// path shortening API
const char* _Nonnull sve4_log_shorten_path(
    char* _Nonnull buffer, size_t buf_size, const char* _Nonnull path,
    const char* _Nullable trim_root_prefix);
