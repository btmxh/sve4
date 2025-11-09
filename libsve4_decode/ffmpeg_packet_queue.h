#pragma once

#include <time.h>

#include "sve4_decode_export.h"

#include "libsve4_utils/defines.h"

#include <libavcodec/packet.h>
#include <libavutil/fifo.h>
#include <tinycthread.h>

#include "error.h"

typedef struct {
  // NOLINTNEXTLINE(misc-include-cleaner)
  mtx_t mutex;
  // NOLINTNEXTLINE(misc-include-cleaner)
  cnd_t push_condvar;
  cnd_t pop_condvar;
  AVFifo* _Nonnull queue;
} sve4_ffmpeg_packet_queue_t;

SVE4_DECODE_EXPORT sve4_decode_error_t sve4_ffmpeg_packet_queue_init(
    sve4_ffmpeg_packet_queue_t* _Nonnull queue, size_t initial_capacity);

SVE4_DECODE_EXPORT sve4_decode_error_t sve4_ffmpeg_packet_queue_push(
    sve4_ffmpeg_packet_queue_t* _Nonnull queue, AVPacket* _Nullable packet,
    const struct timespec* _Nullable time_point, bool force_push);

SVE4_DECODE_EXPORT sve4_decode_error_t
sve4_ffmpeg_packet_queue_pop(sve4_ffmpeg_packet_queue_t* _Nonnull queue,
                             AVPacket* _Nullable* _Nonnull packet,
                             const struct timespec* _Nullable time_point);

SVE4_DECODE_EXPORT sve4_decode_error_t sve4_ffmpeg_packet_queue_is_empty(
    sve4_ffmpeg_packet_queue_t* _Nonnull queue, bool* _Nonnull is_empty);

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_ffmpeg_packet_queue_flush(sve4_ffmpeg_packet_queue_t* _Nonnull queue);

SVE4_DECODE_EXPORT
void sve4_ffmpeg_packet_queue_free(sve4_ffmpeg_packet_queue_t* _Nonnull queue);
