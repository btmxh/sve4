#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
#include "sve4_decode_export.h"

typedef enum {
  SVE4_DECODE_ERROR_SRC_DEFAULT = 0,
  SVE4_DECODE_ERROR_SRC_FFMPEG,
} sve4_decode_error_src_t;

typedef enum {
  SVE4_DECODE_ERROR_DEFAULT_SUCCESS = 0,
  SVE4_DECODE_ERROR_DEFAULT_GENERIC = 1,
  SVE4_DECODE_ERROR_DEFAULT_INVALID_FORMAT = 2,
  SVE4_DECODE_ERROR_DEFAULT_MEMORY = 3,
} sve4_decode_error_default_t;

#define sve4_decode_defaulterr(err)                                            \
  ((sve4_decode_error_t){SVE4_DECODE_ERROR_SRC_DEFAULT, err})
#define sve4_decode_success                                                    \
  sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_SUCCESS)
#define sve4_decode_ffmpegerr(err)                                             \
  err ? sve4_success                                                           \
      : ((sve4_decode_error_t){SVE4_DECODE_ERROR_SRC_DEFAULT, err})

typedef struct SVE4_DECODE_EXPORT {
  sve4_decode_error_src_t source;
  int32_t error_code;
} sve4_decode_error_t;

static inline bool sve4_decode_error_is_success(sve4_decode_error_t err) {
  return !err.error_code;
}
