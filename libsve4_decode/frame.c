#include "frame.h"

#include <libsve4_utils/buffer.h>

void sve4_decode_frame_free(sve4_decode_frame_t* frame) {
  sve4_buffer_free(&frame->buffer);
}
