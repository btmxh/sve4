#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sve4_utils_export.h"

#include "defines.h"

typedef enum {
  SVE4_FMT_SRC_DEFAULT = 0,
  SVE4_FMT_SRC_FFMPEG,
} sve4_fmt_src_t;

typedef enum {
  SVE4_PIXFMT_DEFAULT_UNKNOWN = 0,
  SVE4_PIXFMT_DEFAULT_RGBA8,
} sve4_pixfmt_default_t;

typedef struct SVE4_UTILS_EXPORT {
  sve4_fmt_src_t source;
  int32_t pixfmt;
} sve4_pixfmt_t;

#define sve4_pixfmt_default(val)                                               \
  (sve4_pixfmt_t) {                                                            \
    SVE4_FMT_SRC_DEFAULT, val                                                  \
  }
#define sve4_pixfmt_ffmpeg(val)                                                \
  (sve4_pixfmt_t) {                                                            \
    SVE4_FMT_SRC_FFMPEG, val                                                   \
  }

typedef enum {
  SVE4_SAMPLEFMT_DEFAULT_UNKNOWN = 0,
  SVE4_SAMPLEFMT_DEFAULT_S16,
} sve4_samplefmt_default_t;

typedef struct SVE4_UTILS_EXPORT {
  sve4_fmt_src_t source;
  int32_t format;
} sve4_samplefmt_t;

typedef enum {
  SVE4_GENERICFMT_DEFAULT_UNKNOWN = 0,
} sve4_genericfmt_default_t;

typedef struct SVE4_UTILS_EXPORT {
  sve4_fmt_src_t source;
  int32_t format;
} sve4_genericfmt_t;

typedef enum {
  SVE4_PIXFMT,
  SVE4_SAMPLEFMT,
  SVE4_GENERICFMT,
} sve4_fmt_kind_t;

typedef struct SVE4_UTILS_EXPORT {
  sve4_fmt_kind_t kind;
  union {
    sve4_pixfmt_t pixfmt;
    sve4_samplefmt_t samplefmt;
    sve4_genericfmt_t genericfmt;
  };
} sve4_fmt_t;

SVE4_UTILS_EXPORT
const char* _Nonnull sve4_pixfmt_to_string(sve4_pixfmt_t pixfmt);
SVE4_UTILS_EXPORT
const char* _Nonnull sve4_samplefmt_to_string(sve4_samplefmt_t samplefmt);
SVE4_UTILS_EXPORT
const char* _Nonnull sve4_genericfmt_to_string(sve4_genericfmt_t genericfmt);

SVE4_UTILS_EXPORT
size_t sve4_pixfmt_num_planes(sve4_pixfmt_t pixfmt);
SVE4_UTILS_EXPORT
size_t sve4_pixfmt_linesize(sve4_pixfmt_t pixfmt, size_t plane, size_t width,
                            size_t align);
