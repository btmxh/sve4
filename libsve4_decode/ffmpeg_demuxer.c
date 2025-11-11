#include "ffmpeg_demuxer.h"

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "libsve4_log/api.h"
#include "libsve4_utils/buffer.h"

#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>

#include "decoder.h"
#include "error.h"
#include "ffmpeg_decoder.h"
#include "ffmpeg_demuxer_thread.h"
#include "ffmpeg_packet_queue.h"

static void demuxer_destructor(char* mem) {
  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)(void*)mem;
  sve4_log_debug("ffmpeg: destroying demuxer %p", (void*)demuxer);
  int thrd_return = 0;
  // NOLINTNEXTLINE(misc-include-cleaner)
  atomic_store(&demuxer->running, false);
  if (demuxer->use_thread) {
    sve4_log_debug("ffmpeg: joining demuxer %p packet thread", (void*)demuxer);
    // NOLINTNEXTLINE(misc-include-cleaner)
    if (thrd_join(demuxer->packet_thread, &thrd_return) != thrd_success)
      sve4_log_error("Failed to join packet thread in demuxer destructor");
    else if (thrd_return != 0)
      sve4_log_error("Packet thread exited with error code %d", thrd_return);
  }
  // these decoders hold a reference to the demuxer, so they must be freed first
  assert(demuxer->first_decoder == NULL && demuxer->last_decoder == NULL);
  // NOLINTNEXTLINE(misc-include-cleaner)
  mtx_destroy(&demuxer->decoder_linked_list_mtx);
  sve4_log_debug("ffmpeg: closing demuxer %p (AVFormatContext %p)",
                 (void*)demuxer, (void*)demuxer->ctx);
  avformat_close_input(&demuxer->ctx);
}

sve4_decode_error_t sve4_decode_ffmpeg_open_demuxer(
    sve4_buffer_ref_t* _Nonnull demuxer_ref,
    const sve4_decode_decoder_config_t* _Nonnull config) {
  sve4_decode_error_t err;
  *demuxer_ref = sve4_buffer_create(NULL, sizeof(sve4_decode_ffmpeg_demuxer_t),
                                    demuxer_destructor);
  if (!*demuxer_ref) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }

  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)sve4_buffer_get_data(*demuxer_ref);
  demuxer->ctx = NULL;
  demuxer->first_decoder = demuxer->last_decoder = NULL;
  demuxer->demuxer_thread_interval = 0;
  demuxer->seek_request = -1;
  demuxer->use_thread = false;
  demuxer->reach_eof = false;

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_init(&demuxer->decoder_linked_list_mtx, mtx_plain) != thrd_success) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
    goto fail;
  }

  sve4_log_debug("ffmpeg: initializing demuxer %p for url %s", (void*)demuxer,
                 config->url);
  err = sve4_decode_ffmpegerr(avformat_open_input(
      &demuxer->ctx, config->url,
      config->avformat_open_input ? config->avformat_open_input->fmt : NULL,
      config->avformat_open_input ? config->avformat_open_input->options
                                  : NULL));
  if (!sve4_decode_error_is_success(err))
    goto fail;

  err = sve4_decode_ffmpegerr(avformat_find_stream_info(
      demuxer->ctx, config->avformat_find_stream_info
                        ? config->avformat_find_stream_info->options
                        : NULL));
  if (!sve4_decode_error_is_success(err))
    goto fail;

  sve4_log_debug("ffmpeg: dumping demuxer %p media info", (void*)demuxer);
  av_dump_format(demuxer->ctx, 0, config->url, 0);

  return sve4_decode_success;
fail:
  sve4_buffer_free(demuxer_ref);
  return err;
}

static sve4_decode_error_t
init_thread_demuxer(sve4_decode_ffmpeg_demuxer_t* demuxer) {
  if (demuxer->use_thread)
    return sve4_decode_success;
  demuxer->use_thread = true;
  sve4_log_debug("ffmpeg: initializing demuxer thread for demuxer %p",
                 (void*)demuxer);
  atomic_store(&demuxer->running, true);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (thrd_create(&demuxer->packet_thread, sve4_demuxer_thread_main, demuxer) !=
      thrd_success) {
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
  }

  return sve4_decode_success;
}

static sve4_decode_media_type_t
ffmpeg_media_type_to_sve4(enum AVMediaType ffmpeg_type) {
  switch (ffmpeg_type) {
  case AVMEDIA_TYPE_VIDEO:
    return SVE4_DECODE_MEDIA_TYPE_VIDEO;
  case AVMEDIA_TYPE_AUDIO:
    return SVE4_DECODE_MEDIA_TYPE_AUDIO;
  case AVMEDIA_TYPE_SUBTITLE:
    return SVE4_DECODE_MEDIA_TYPE_SUBTITLE;
  default:;
  }
  return SVE4_DECODE_MEDIA_TYPE_UNKNOWN;
}

void sve4_decode_ffmpeg_demuxer_get_streams(sve4_buffer_ref_t demuxer_ref,
                                            sve4_decode_stream_t* streams,
                                            size_t* nb_streams) {
  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)sve4_buffer_get_data(demuxer_ref);
  assert(demuxer);
  for (size_t i = 0; i < demuxer->ctx->nb_streams; ++i) {
    if (nb_streams && i >= *nb_streams)
      break;
    AVStream* stream = demuxer->ctx->streams[i];
    if (streams) {
      streams[i].type = ffmpeg_media_type_to_sve4(stream->codecpar->codec_type);
      AVDictionaryEntry* entry =
          av_dict_get(stream->metadata, "title", NULL, 0);
      streams[i].title = entry ? entry->value : NULL;
      entry = av_dict_get(stream->metadata, "language", NULL, 0);
      streams[i].language = entry ? entry->value : NULL;
      streams[i].is_default = stream->disposition & AV_DISPOSITION_DEFAULT;
      streams[i].is_forced = stream->disposition & AV_DISPOSITION_FORCED;
      streams[i].opaque = stream;
    }
  }
  if (nb_streams)
    *nb_streams = demuxer->ctx->nb_streams;
}

sve4_decode_error_t
sve4_decode_ffmpeg_demuxer_add_decoder(sve4_decode_ffmpeg_demuxer_t* demuxer,
                                       sve4_decode_ffmpeg_decoder_t* decoder) {
  sve4_decode_error_t err = sve4_decode_success;
  bool init_first_decoder = false;
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_lock(&demuxer->decoder_linked_list_mtx) != thrd_success) {
    sve4_log_error("Failed to lock decoder linked list mutex in decoder close");
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
    goto ret;
  }

  sve4_decode_ffmpeg_decoder_t* first_decoder = demuxer->first_decoder;
  if (first_decoder) {
    if (first_decoder == demuxer->last_decoder) {
      sve4_log_debug("ffmpeg: demuxer is already attached to a decoder, "
                     "initializing packet queue for this decoder %p",
                     (void*)first_decoder);
      err = sve4_decode_ffmpeg_decoder_inner_init_packet_queue(first_decoder);
      if (!sve4_decode_error_is_success(err))
        goto ret;
      init_first_decoder = true;
    }

    sve4_log_debug("ffmpeg: initializing packet queue for new decoder %p",
                   (void*)decoder);
    err = sve4_decode_ffmpeg_decoder_inner_init_packet_queue(decoder);
    if (!sve4_decode_error_is_success(err))
      goto ret;
  }

  demuxer->last_decoder ? (demuxer->last_decoder->next = decoder)
                        : (demuxer->first_decoder = decoder);
  demuxer->last_decoder = decoder;

ret:
  if (!sve4_decode_error_is_success(err)) {
    sve4_ffmpeg_packet_queue_free(&decoder->packet_queue);
    if (init_first_decoder)
      sve4_ffmpeg_packet_queue_free(&demuxer->first_decoder->packet_queue);
  }
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_unlock(&demuxer->decoder_linked_list_mtx) != thrd_success)
    sve4_log_error(
        "Failed to unlock decoder linked list mutex in decoder close");
  return err;
}

sve4_decode_error_t sve4_decode_ffmpeg_demuxer_read_packet(
    sve4_buffer_ref_t demuxer_ref, sve4_decode_ffmpeg_decoder_t* decoder,
    AVPacket** packet, const struct timespec* deadline) {
  sve4_decode_error_t err = sve4_decode_success;
  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)sve4_buffer_get_data(demuxer_ref);
  if (demuxer->first_decoder != demuxer->last_decoder) {
    err = init_thread_demuxer(demuxer);
    if (!sve4_decode_error_is_success(err))
      return err;
  }

  if (demuxer->use_thread) {
    return sve4_ffmpeg_packet_queue_pop(&decoder->packet_queue, packet,
                                        deadline);
  }

  *packet = av_packet_alloc();
  if (!*packet)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);

  int ffmpeg_err = av_read_frame(demuxer->ctx, *packet);
  if (ffmpeg_err == AVERROR_EOF) {
    av_packet_free(packet);
    if (demuxer->reach_eof)
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_EOF);
    demuxer->reach_eof = true;
    return sve4_decode_success;
  }

  err = sve4_decode_ffmpegerr(ffmpeg_err);
  if (!sve4_decode_error_is_success(err))
    av_packet_free(packet);

  return err;
}

sve4_decode_error_t
sve4_decode_ffmpeg_demuxer_seek(sve4_buffer_ref_t _Nonnull demuxer_ref,
                                int64_t pos) {
  sve4_decode_ffmpeg_demuxer_t* demuxer =
      (sve4_decode_ffmpeg_demuxer_t*)sve4_buffer_get_data(demuxer_ref);
  sve4_log_debug("ffmpeg: demuxer %p seeking to position %" PRId64
                 " ns. note that every decoder attached to this demuxer will "
                 "also seek to this position",
                 (void*)demuxer, pos);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  pos = pos / ((int)1e9 / AV_TIME_BASE);

  if (demuxer->use_thread) {
    // signal the demuxer thread to perform the seek
    atomic_store(&demuxer->seek_request, pos);
    return sve4_decode_success;
  }

  demuxer->reach_eof = false;
  return sve4_decode_ffmpegerr(
      av_seek_frame(demuxer->ctx, -1, pos, AVSEEK_FLAG_BACKWARD));
}
