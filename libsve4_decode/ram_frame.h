#pragma once

#include <stdint.h>

#include <libsve4_utils/buffer.h>
#include <libsve4_utils/defines.h>
#include <libsve4_utils/formats.h>
#include <sve4_decode_export.h>

#include "error.h"
#include "frame.h"

enum { SVE4_DECODE_RAM_FRAME_MAX_PLANES = 8 };

typedef struct {
  uint8_t* _Nullable data[SVE4_DECODE_RAM_FRAME_MAX_PLANES];
  size_t linesizes[SVE4_DECODE_RAM_FRAME_MAX_PLANES];
  char contiguous_data[];
} sve4_decode_ram_frame_t;

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_alloc_ram_frame(sve4_decode_frame_t* _Nonnull frame,
                            sve4_allocator_t* _Nullable allocator,
                            sve4_pixfmt_t fmt, size_t width, size_t height,
                            const size_t* _Nonnull plane_align);
