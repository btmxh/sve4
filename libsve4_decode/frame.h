#pragma once

#include <stdint.h>

#include "buffer.h"
#include "formats.h"

typedef enum {
  SVE4_DECODE_FRAME_KIND_NONE = 0,
  SVE4_DECODE_FRAME_KIND_RAM_FRAME,
  SVE4_DECODE_FRAME_KIND_VULKAN,
  SVE4_DECODE_FRAME_KIND_AVFRAME,
} sve4_decode_frame_kind_t;

typedef struct {
  sve4_buffer_ref_t _Nullable buffer;
  sve4_decode_frame_kind_t kind;
  sve4_fmt_t format;
  int64_t pts, duration;
} sve4_decode_frame_t;

void sve4_decode_frame_free(sve4_decode_frame_t* _Nullable frame);
