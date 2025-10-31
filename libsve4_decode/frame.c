#include "frame.h"

#include "buffer.h"

void sve4_decode_frame_free(sve4_decode_frame_t* frame) {
  sve4_buffer_free(&frame->buffer);
}
