#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "libsve4_utils/error.h"

#include <libsve4_utils/defines.h>
#include <sve4_decode_export.h>

typedef enum {
  SVE4_DECODE_ERROR_SRC_DEFAULT = 0,
  SVE4_DECODE_ERROR_SRC_FFMPEG,
} sve4_decode_error_src_t;

typedef enum {
  SVE4_DECODE_ERROR_DEFAULT_SUCCESS = 0,
  SVE4_DECODE_ERROR_DEFAULT_GENERIC,
  SVE4_DECODE_ERROR_DEFAULT_INVALID_FORMAT,
  SVE4_DECODE_ERROR_DEFAULT_MEMORY,
  SVE4_DECODE_ERROR_DEFAULT_BACKEND_MISSING,
  SVE4_DECODE_ERROR_DEFAULT_UNIMPLEMENTED,
  SVE4_DECODE_ERROR_DEFAULT_IO,
  SVE4_DECODE_ERROR_DEFAULT_EOF,
  SVE4_DECODE_ERROR_DEFAULT_THREADS,
  SVE4_DECODE_ERROR_DEFAULT_TIMEOUT,
  SVE4_DECODE_ERROR_DEFAULT_USER_CANCELLED,
  SVE4_DECODE_ERROR_DEFAULT_CODEC_NOT_FOUND,
} sve4_decode_error_default_t;

#ifdef NDEBUG
#define sve4_decode_make_error(src, err) ((sve4_decode_error_t){src, err})
#else
#define sve4_decode_make_error(src, err)                                       \
  ((sve4_decode_error_t){src, err, SVE4_ERROR_TRACE_HERE})
#endif

#define sve4_decode_defaulterr(err)                                            \
  sve4_decode_make_error(SVE4_DECODE_ERROR_SRC_DEFAULT, err)
#define sve4_decode_success                                                    \
  sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_SUCCESS)
#define sve4_decode_ffmpegerr(err)                                             \
  err == 0 ? sve4_decode_success                                               \
           : sve4_decode_make_error(SVE4_DECODE_ERROR_SRC_FFMPEG, err)

typedef struct {
  sve4_decode_error_src_t source;
  int32_t error_code;
#ifndef NDEBUG
  sve4_error_trace_t trace;
#endif
} sve4_decode_error_t;

static inline bool sve4_decode_error_is_success(sve4_decode_error_t err) {
  return !err.error_code;
}
