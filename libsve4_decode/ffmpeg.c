#include "ffmpeg.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "libsve4_decode/ffmpeg_decoder.h"
#include "libsve4_decode/ffmpeg_demuxer.h"
#include "libsve4_log/api.h"
#include "libsve4_utils/allocator.h"
#include "libsve4_utils/buffer.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>

#include "ffmpeg_packet_queue.h"
#include "frame.h"

// NOLINTNEXTLINE(misc-include-cleaner)
#include <tinycthread.h>

#include "decoder.h"
#include "error.h"

static void inner_decoder_destructor(char* mem) {
  sve4_decode_ffmpeg_decoder_t* decoder =
      (sve4_decode_ffmpeg_decoder_t*)(void*)mem;
  sve4_decode_ffmpeg_close_decoder_inner(decoder);
}

static sve4_decode_error_t
ffmpeg_get_frame(sve4_decode_decoder_t* _Nonnull decoder,
                 sve4_decode_frame_t* _Nonnull frame,
                 const struct timespec* _Nullable deadline) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  sve4_decode_ffmpeg_decoder_t* inner_decoder =
      (sve4_decode_ffmpeg_decoder_t*)sve4_buffer_get_data(decoder->data);
#pragma GCC diagnostic pop
  return sve4_decode_ffmpeg_decoder_inner_get_frame(inner_decoder, frame,
                                                    deadline);
}

static sve4_decode_error_t ffmpeg_seek(sve4_decode_decoder_t* decoder,
                                       int64_t pos) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  sve4_decode_ffmpeg_decoder_t* inner_decoder =
      (sve4_decode_ffmpeg_decoder_t*)sve4_buffer_get_data(decoder->data);
#pragma GCC diagnostic pop
  return sve4_decode_ffmpeg_decoder_inner_seek(inner_decoder, pos);
}

sve4_decode_error_t
sve4_decode_ffmpeg_open_decoder(sve4_decode_decoder_t* decoder,
                                const sve4_decode_decoder_config_t* config) {
  (void)decoder;
  (void)config;
  sve4_decode_error_t err;
  sve4_buffer_ref_t demuxer = config->demuxer;
  sve4_buffer_ref_t inner_decoder_ref = NULL;
  sve4_decode_stream_t* streams = NULL;
  size_t nb_streams = 0;

  if (!demuxer) {
    sve4_log_debug(
        "ffmpeg: demuxer not provided, opening new demuxer for url %s",
        config->url);
    err = sve4_decode_ffmpeg_open_demuxer(&demuxer, config);
    if (!sve4_decode_error_is_success(err))
      goto fail;
  }

  inner_decoder_ref = sve4_buffer_create(
      NULL, sizeof(sve4_decode_ffmpeg_decoder_t), inner_decoder_destructor);
  if (!inner_decoder_ref) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }

  sve4_decode_ffmpeg_decoder_t* inner_decoder =
      (sve4_decode_ffmpeg_decoder_t*)sve4_buffer_get_data(inner_decoder_ref);

  sve4_decode_ffmpeg_demuxer_get_streams(demuxer, NULL, &nb_streams);
  streams =
      sve4_malloc(config->allocator, nb_streams * sizeof(sve4_decode_stream_t));
  if (!streams) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }
  sve4_decode_ffmpeg_demuxer_get_streams(demuxer, streams, &nb_streams);

  sve4_log_debug("ffmpeg: found %zu streams in demuxer, using user callback to "
                 "choose which stream to decode",
                 nb_streams);
  size_t stream_index = sve4_decode_stream_choose(
      decoder, &config->stream_chooser, streams, nb_streams);
  if (stream_index > nb_streams) {
    sve4_log_error(
        "ffmpeg: user returns invalid stream index %zu (nb_streams=%zu)",
        stream_index, nb_streams);
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_USER_CANCELLED);
    goto fail;
  }

  sve4_free(config->allocator, streams);
  streams = NULL;

  sve4_log_debug("ffmpeg: selected stream index %zu", stream_index);
  err = sve4_decode_ffmpeg_open_decoder_inner(inner_decoder, demuxer,
                                              stream_index, config);
  if (!sve4_decode_error_is_success(err))
    goto fail;

  decoder->demuxer = demuxer;
  decoder->data = inner_decoder_ref;
  decoder->get_frame = ffmpeg_get_frame;
  decoder->seek = ffmpeg_seek;

  return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_SUCCESS);
fail:
  sve4_free(config->allocator, streams);
  sve4_buffer_free(&inner_decoder_ref);
  if (demuxer != config->demuxer)
    sve4_buffer_free(&demuxer);
  return err;
}

static const AVCodec* _Nullable pick_codec(
    const sve4_decode_decoder_config_t* _Nonnull config,
    const AVCodecParameters* codecpar) {
  (void)config;
  enum AVCodecID codec_id = codecpar->codec_id;
  const AVCodec* codec = avcodec_find_decoder(codec_id);
  return codec;
}

sve4_decode_error_t sve4_decode_ffmpeg_open_decoder_inner(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder,
    sve4_buffer_ref_t _Nonnull demuxer_ref, size_t stream_index,
    const sve4_decode_decoder_config_t* _Nonnull config) {
  decoder->ctx = NULL;
  decoder->demuxer = demuxer_ref;
  decoder->stream_index = stream_index;
  decoder->last_packet_idx = SIZE_MAX;
  decoder->packet_queue_initial_capacity =
      config->packet_queue_initial_capacity
          ? config->packet_queue_initial_capacity
          // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
          : 8;
  decoder->frame_allocator = config->frame_allocator;

  sve4_decode_error_t err;
  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)sve4_buffer_get_data(demuxer_ref);

  const AVCodec* codec =
      config->avcodec_open2
          ? config->avcodec_open2->codec
          : pick_codec(config, demuxer->ctx->streams[stream_index]->codecpar);
  if (!codec) {
    sve4_log_error("ffmpeg: failed to find codec for stream index %zu",
                   stream_index);
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_CODEC_NOT_FOUND);
    goto fail;
  }

  sve4_log_debug("ffmpeg: picked codec %s", codec->name);
  if (!(decoder->ctx = avcodec_alloc_context3(codec))) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }

  sve4_log_debug(
      "ffmpeg: setting codec parameters for decoder %p (AVCodecContext* %p)",
      (void*)decoder, (void*)decoder->ctx);
  err = sve4_decode_defaulterr(avcodec_parameters_to_context(
      decoder->ctx, demuxer->ctx->streams[stream_index]->codecpar));
  if (!sve4_decode_error_is_success(err))
    goto fail;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  if (config->setup_codec_context && config->setup_codec_context->setup)
    config->setup_codec_context->setup(decoder->ctx,
                                       config->setup_codec_context->user_ptr);
#pragma GCC diagnostic pop

  sve4_log_debug("ffmpeg: opening codec context for decoder %p",
                 (void*)decoder);
  err = sve4_decode_defaulterr(avcodec_open2(
      decoder->ctx, codec,
      config->avcodec_open2 ? config->avcodec_open2->options : NULL));
  if (!sve4_decode_error_is_success(err))
    goto fail;

  err = sve4_decode_ffmpeg_demuxer_add_decoder(demuxer, decoder);
  if (!sve4_decode_error_is_success(err))
    goto fail;

  return sve4_decode_success;

fail:
  sve4_decode_ffmpeg_close_decoder_inner(decoder);
  return err;
}

void sve4_decode_ffmpeg_close_decoder_inner(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder) {
  if (!decoder->demuxer)
    return;
  sve4_log_debug("ffmpeg: closing decoder %p", (void*)decoder);
  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)sve4_buffer_get_data(decoder->demuxer);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_lock(&demuxer->decoder_linked_list_mtx) != thrd_success) {
    sve4_log_error("Failed to lock decoder linked list mutex in decoder close");
    return;
  }

  decoder->prev ? (decoder->prev->next = decoder->next)
                : (demuxer->first_decoder = decoder->next);
  decoder->next ? (decoder->next->prev = decoder->prev)
                : (demuxer->last_decoder = decoder->prev);

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_unlock(&demuxer->decoder_linked_list_mtx) != thrd_success)
    sve4_log_error(
        "Failed to unlock decoder linked list mutex in decoder close");

  if (decoder->packet_queue.queue)
    sve4_ffmpeg_packet_queue_free(&decoder->packet_queue);
  sve4_buffer_unref(decoder->demuxer);
  avcodec_free_context(&decoder->ctx);
}
