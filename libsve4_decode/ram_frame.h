#pragma once

#include <stdint.h>

#include "buffer.h"
#include "defines.h"
#include "error.h"
#include "formats.h"
#include "frame.h"

enum { SVE4_DECODE_RAM_FRAME_MAX_PLANES = 8 };

typedef struct SVE4_EXPORT {
  uint8_t* _Nullable data[SVE4_DECODE_RAM_FRAME_MAX_PLANES];
  size_t linesizes[SVE4_DECODE_RAM_FRAME_MAX_PLANES];
  char contiguous_data[];
} sve4_decode_ram_frame_t;

SVE4_EXPORT
sve4_decode_error_t
sve4_decode_alloc_ram_frame(sve4_decode_frame_t* _Nonnull frame,
                            sve4_allocator_t* _Nullable allocator,
                            sve4_pixfmt_t fmt, size_t width, size_t height,
                            size_t* _Nonnull plane_align);
