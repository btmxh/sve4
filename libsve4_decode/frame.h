#pragma once

#include <stdint.h>

#include <libsve4_utils/buffer.h>
#include <libsve4_utils/formats.h>
#include <sve4_decode_export.h>

typedef enum {
  SVE4_DECODE_FRAME_KIND_NONE = 0,
  SVE4_DECODE_FRAME_KIND_RAM_FRAME,
  SVE4_DECODE_FRAME_KIND_VULKAN,
  SVE4_DECODE_FRAME_KIND_AVFRAME,
} sve4_decode_frame_kind_t;

typedef struct SVE4_DECODE_EXPORT {
  sve4_buffer_ref_t _Nullable buffer;
  sve4_decode_frame_kind_t kind;
  sve4_fmt_t format;
  int64_t pts, duration;
  size_t width, height;
} sve4_decode_frame_t;

SVE4_UTILS_EXPORT
void sve4_decode_frame_free(sve4_decode_frame_t* _Nullable frame);
