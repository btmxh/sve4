#include "ram_frame.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <libsve4_utils/allocator.h>
#include <libsve4_utils/buffer.h>
#include <libsve4_utils/formats.h>

#include "error.h"
#include "frame.h"

sve4_decode_error_t sve4_decode_alloc_ram_frame(sve4_decode_frame_t* frame,
                                                sve4_allocator_t* allocator,
                                                sve4_pixfmt_t fmt, size_t width,
                                                size_t height,
                                                const size_t* plane_align) {
  assert(width && height && plane_align);

  size_t num_planes = sve4_pixfmt_num_planes(fmt);
  assert(num_planes && num_planes <= SVE4_DECODE_RAM_FRAME_MAX_PLANES);

  size_t align = 0;
  size_t size = sizeof(sve4_decode_ram_frame_t);
  size_t offsets[SVE4_DECODE_RAM_FRAME_MAX_PLANES] = {0};
  size_t linesizes[SVE4_DECODE_RAM_FRAME_MAX_PLANES] = {0};
  for (size_t i = 0; i < num_planes; ++i) {
    if (plane_align[i] > align)
      align = plane_align[i];
    size = sve4_align_up(size, plane_align[i]);
    offsets[i] = size;
    linesizes[i] = sve4_pixfmt_linesize(fmt, i, width, plane_align[i]);
    assert(linesizes[i]);
    size += linesizes[i] * height;
  }

  sve4_buffer_ref_t buffer = sve4_buffer_create(allocator, size, NULL);
  if (!buffer) {
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
  }

  sve4_decode_ram_frame_t* ram_frame = sve4_buffer_get_data(buffer);
  memcpy(&ram_frame->linesizes, linesizes, sizeof(size_t) * num_planes);

  for (size_t i = 0; i < num_planes; ++i) {
    ram_frame->data[i] = &((uint8_t*)ram_frame)[offsets[i]];
  }

  frame->width = width;
  frame->height = height;
  frame->buffer = buffer;
  frame->format = (sve4_fmt_t){.kind = SVE4_PIXFMT, .pixfmt = fmt};
  frame->kind = SVE4_DECODE_FRAME_KIND_RAM_FRAME;
  return sve4_decode_success;
}
