#pragma once

#include "sve4_decode_export.h"

#include "libsve4_decode/decoder.h"
#include "libsve4_decode/error.h"
#include "libsve4_utils/buffer.h"
#include "libsve4_utils/defines.h"

#include <libavcodec/avcodec.h>

#include "ffmpeg_packet_queue.h"

typedef struct sve4_decode_ffmpeg_decoder_t {
  sve4_buffer_ref_t _Nonnull demuxer;
  AVCodecContext* _Nullable ctx;
  struct sve4_decode_ffmpeg_decoder_t* _Nullable prev;
  struct sve4_decode_ffmpeg_decoder_t* _Nullable next;
  size_t stream_index;
  size_t packet_queue_initial_capacity;
  sve4_ffmpeg_packet_queue_t packet_queue;
  size_t last_packet_idx;
  sve4_allocator_t* _Nullable frame_allocator;
} sve4_decode_ffmpeg_decoder_t;

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_open_decoder_inner(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder,
    sve4_buffer_ref_t _Nonnull demuxer, size_t stream_index,
    const sve4_decode_decoder_config_t* _Nonnull config);

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_decoder_inner_init_packet_queue(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder);

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_decoder_inner_get_frame(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder,
    sve4_decode_frame_t* _Nonnull frame,
    const struct timespec* _Nullable deadline);

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_decoder_inner_seek(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder, int64_t pos);

SVE4_DECODE_EXPORT
void sve4_decode_ffmpeg_close_decoder_inner(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder);
