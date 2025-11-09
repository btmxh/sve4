#pragma once

#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#include "sve4_decode_export.h"

#include "libsve4_decode/decoder.h"
#include "libsve4_decode/error.h"
#include "libsve4_utils/buffer.h"
#include "libsve4_utils/defines.h"

#include <libavformat/avformat.h>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tinycthread.h>

typedef struct {
  AVFormatContext* _Nullable ctx;
  struct sve4_decode_ffmpeg_decoder_t* _Nullable first_decoder;
  struct sve4_decode_ffmpeg_decoder_t* _Nullable last_decoder;
  // NOLINTNEXTLINE(misc-include-cleaner)
  mtx_t decoder_linked_list_mtx;
  bool use_thread;
  // NOLINTNEXTLINE(misc-include-cleaner)
  thrd_t packet_thread;
  // NOLINTNEXTLINE(misc-include-cleaner)
  atomic_bool running;
  int64_t demuxer_thread_interval;
  // NOLINTNEXTLINE(misc-include-cleaner)
  atomic_int_fast64_t seek_request; // -1 => no seek, >= 0 -> seek requested
} sve4_decode_ffmpeg_demuxer_t;

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_open_demuxer(
    sve4_buffer_ref_t _Nullable* _Nonnull demuxer_ref,
    const sve4_decode_decoder_config_t* _Nonnull config);

SVE4_DECODE_EXPORT
void sve4_decode_ffmpeg_demuxer_get_streams(
    sve4_buffer_ref_t _Nonnull demuxer_ref,
    sve4_decode_stream_t* _Nullable streams, size_t* _Nullable nb_streams);

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_demuxer_add_decoder(
    sve4_decode_ffmpeg_demuxer_t* _Nonnull demuxer_ref,
    struct sve4_decode_ffmpeg_decoder_t* _Nonnull decoder);

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_ffmpeg_demuxer_read_packet(
    sve4_buffer_ref_t _Nonnull demuxer_ref,
    struct sve4_decode_ffmpeg_decoder_t* _Nullable decoder,
    AVPacket* _Nullable* _Nonnull packet,
    const struct timespec* _Nullable deadline);

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_ffmpeg_demuxer_seek(sve4_buffer_ref_t _Nonnull demuxer_ref,
                                int64_t pos);
