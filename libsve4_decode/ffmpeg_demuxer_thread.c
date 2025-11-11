#include "ffmpeg_demuxer_thread.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#include "libsve4_log/api.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "libsve4_utils/defines.h"

#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>

#include "error.h"
#include "ffmpeg_decoder.h"
#include "ffmpeg_demuxer.h"
#include "ffmpeg_packet_queue.h"

typedef enum {
  DT_ERROR_SUCCESS = 0,
  DT_ERROR_MEMORY,
  DT_ERROR_THREADS,
} thread_error_t;

typedef struct {
  sve4_decode_ffmpeg_demuxer_t* _Nonnull demuxer;
  int64_t demuxer_interval_ns;
  AVPacket* _Nonnull current_packet;
  size_t current_packet_idx;
  bool reach_eof, has_pending_packet; // since NULL packet means EOF
} thread_ctx_t;

static thread_error_t thread_ctx_init(thread_ctx_t* ctx,
                                      sve4_decode_ffmpeg_demuxer_t* demuxer) {
  ctx->demuxer = demuxer;
  ctx->demuxer_interval_ns =
      demuxer->demuxer_thread_interval
          ? demuxer->demuxer_thread_interval
          // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
          : (int64_t)10e6;
  ctx->has_pending_packet = ctx->reach_eof = false;
  ctx->current_packet = av_packet_alloc();
  ctx->current_packet_idx = SIZE_MAX;
  if (!ctx->current_packet)
    return DT_ERROR_MEMORY;
  return DT_ERROR_SUCCESS;
}

static void thread_ctx_free(thread_ctx_t* ctx) {
  av_packet_free(&ctx->current_packet);
}

static bool needs_force_push(thread_ctx_t* ctx) {
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_lock(&ctx->demuxer->decoder_linked_list_mtx) != thrd_success) {
    sve4_log_error("Failed to lock decoder linked list mutex in demuxer "
                   "packet thread");
    return false;
  }

  bool empty = false;
  for (sve4_decode_ffmpeg_decoder_t* decoder = ctx->demuxer->first_decoder;
       decoder != NULL; decoder = decoder->next) {
    bool queue_empty = false;
    sve4_decode_error_t err =
        sve4_ffmpeg_packet_queue_is_empty(&decoder->packet_queue, &queue_empty);
    if (!sve4_decode_error_is_success(err))
      continue;
    if (queue_empty) {
      empty = true;
      break;
    }
  }

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_unlock(&ctx->demuxer->decoder_linked_list_mtx) != thrd_success)
    sve4_log_error("Failed to unlock decoder linked list mutex in demuxer "
                   "packet thread");

  return empty;
}

static int read_frame(thread_ctx_t* ctx) {
  if (ctx->reach_eof || ctx->has_pending_packet)
    return DT_ERROR_SUCCESS;
  int err = av_read_frame(ctx->demuxer->ctx, ctx->current_packet);
  if (err < 0)
    return err;
  ctx->has_pending_packet = err != AVERROR_EOF || !ctx->reach_eof;
  if (err == AVERROR_EOF) {
    if (!ctx->reach_eof)
      sve4_log_debug("ffmpeg_demux_thread: reached EOF");
    ctx->reach_eof = true;
    av_packet_unref(ctx->current_packet);
  }

  if (ctx->current_packet->data)
    sve4_log_debug("ffmpeg_demux_thread: read packet idx %zu (stream "
                   "%d, size %d)",
                   ctx->current_packet_idx + 1,
                   ctx->current_packet->stream_index,
                   ctx->current_packet->size);
  ++ctx->current_packet_idx;
  return DT_ERROR_SUCCESS;
}

static int handle_seek_request(thread_ctx_t* ctx, int64_t seek_req) {
  assert(seek_req >= 0);
  // flush everything
  ctx->has_pending_packet = ctx->reach_eof = false;
  av_packet_unref(ctx->current_packet);

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_lock(&ctx->demuxer->decoder_linked_list_mtx) != thrd_success) {
    sve4_log_error("Failed to lock decoder linked list mutex in demuxer "
                   "packet thread");
    return DT_ERROR_THREADS;
  }

  for (sve4_decode_ffmpeg_decoder_t* decoder = ctx->demuxer->first_decoder;
       decoder != NULL; decoder = decoder->next) {
    sve4_decode_error_t err =
        sve4_ffmpeg_packet_queue_clear(&decoder->packet_queue);
    decoder->last_packet_idx = SIZE_MAX;
    if (!sve4_decode_error_is_success(err)) {
      sve4_log_error("Failed to flush packet queue in demuxer packet thread");
      return DT_ERROR_THREADS;
    }
  }

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_unlock(&ctx->demuxer->decoder_linked_list_mtx) != thrd_success)
    sve4_log_error("Failed to unlock decoder linked list mutex in demuxer "
                   "packet thread");

  int err =
      av_seek_frame(ctx->demuxer->ctx, -1, seek_req, AVSEEK_FLAG_BACKWARD);
  if (err < 0) {
    sve4_log_error("Failed to seek in demuxer packet thread");
    return err;
  }

  atomic_store(&ctx->demuxer->seek_request, -1);
  return DT_ERROR_SUCCESS;
}

static void timespec_add_ns(struct timespec* time, int64_t amt) {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  time->tv_sec += amt / 1000000000;
  time->tv_nsec += amt % 1000000000;
  if (time->tv_nsec >= 1000000000) {
    time->tv_sec += 1;
    time->tv_nsec -= 1000000000;
  }
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
}

static struct timespec get_deadline(thread_ctx_t* ctx) {
  struct timespec deadline;
  timespec_get(&deadline, TIME_UTC);
  timespec_add_ns(&deadline, ctx->demuxer_interval_ns);
  return deadline;
}

static int try_send_packet(thread_ctx_t* ctx, bool force_push,
                           struct timespec* deadline, bool* _Nonnull timeout) {
  int err = DT_ERROR_SUCCESS;
  if (!ctx->has_pending_packet)
    return DT_ERROR_SUCCESS;

  if (mtx_lock(&ctx->demuxer->decoder_linked_list_mtx) != thrd_success) {
    sve4_log_error("Failed to lock decoder linked list mutex in demuxer "
                   "packet thread");
    return DT_ERROR_THREADS;
  }

  for (sve4_decode_ffmpeg_decoder_t* decoder = ctx->demuxer->first_decoder;
       decoder != NULL; decoder = decoder->next) {
    // if not a flush packet and not for this stream, skip
    if (ctx->current_packet->data &&
        (size_t)ctx->current_packet->stream_index != decoder->stream_index)
      continue;
    // if this packet has already been sent to this decoder, skip
    if (ctx->current_packet_idx == decoder->last_packet_idx)
      continue;

    AVPacket* packet = NULL;
    if (ctx->current_packet)
      if (!(packet = av_packet_clone(ctx->current_packet))) {
        err = DT_ERROR_MEMORY;
        break;
      }

    sve4_log_debug(
        "ffmpeg_demux_thread: sending packet %p (idx %zu) to decoder %p"
        "(stream %zu)",
        (void*)packet, ctx->current_packet_idx, (void*)decoder,
        decoder->stream_index);
    sve4_decode_error_t push_err = sve4_ffmpeg_packet_queue_push(
        &decoder->packet_queue, packet, deadline, force_push);
    if (sve4_decode_error_is_success(push_err)) {
      decoder->last_packet_idx = ctx->current_packet_idx;
      continue;
    }
    if (push_err.source == SVE4_DECODE_ERROR_SRC_DEFAULT &&
        push_err.error_code == SVE4_DECODE_ERROR_DEFAULT_TIMEOUT) {
      *timeout = true;
      break;
    }
    err = push_err.error_code;
    break;
  }

  if (!(ctx->has_pending_packet = *timeout)) {
    av_packet_unref(ctx->current_packet);
  }

  if (mtx_unlock(&ctx->demuxer->decoder_linked_list_mtx) != thrd_success)
    sve4_log_error("Failed to unlock decoder linked list mutex in demuxer "
                   "packet thread");

  return err;
}

int sve4_demuxer_thread_main(void* user_ptr) {
  int err = DT_ERROR_SUCCESS;
  thread_ctx_t ctx = {0};
  if ((thread_ctx_init(&ctx, (sve4_decode_ffmpeg_demuxer_t*)user_ptr)) !=
      DT_ERROR_SUCCESS)
    goto ret;

  // NOLINTNEXTLINE(misc-include-cleaner)
  while (atomic_load(&ctx.demuxer->running)) {
    // NOLINTNEXTLINE(misc-include-cleaner)
    int64_t seek_req = atomic_load(&ctx.demuxer->seek_request);
    if (seek_req >= 0) {
      if ((err = handle_seek_request(&ctx, seek_req)) != DT_ERROR_SUCCESS)
        goto ret;
      continue;
    }

    bool force_push = needs_force_push(&ctx);
    if ((err = read_frame(&ctx)) != DT_ERROR_SUCCESS)
      goto ret;

    struct timespec deadline = get_deadline(&ctx);
    bool timeout = false;
    try_send_packet(&ctx, force_push, &deadline, &timeout);

    if (!timeout) {
      ctx.has_pending_packet = true;
      av_packet_unref(ctx.current_packet);
    }
  }

ret:
  thread_ctx_free(&ctx);
  return err;
}
