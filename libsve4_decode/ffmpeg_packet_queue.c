#include "ffmpeg_packet_queue.h"

#include <string.h>
#include <time.h>

#include "libsve4_log/api.h"

#include <libavcodec/packet.h>
#include <libavutil/fifo.h>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tinycthread.h>

#include "error.h"

static sve4_decode_error_t thread_err_to_sve4(int err) {
  switch (err) {
  // NOLINTNEXTLINE(misc-include-cleaner)
  case thrd_success:
    return sve4_decode_success;
  // NOLINTNEXTLINE(misc-include-cleaner)
  case thrd_timedout:
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_TIMEOUT);
  default:;
  }
  return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
}

sve4_decode_error_t
sve4_ffmpeg_packet_queue_init(sve4_ffmpeg_packet_queue_t* _Nonnull queue,
                              size_t initial_capacity) {
  memset(queue, 0, sizeof *queue);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_init(&queue->mutex, mtx_timed) != thrd_success)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (cnd_init(&queue->push_condvar) != thrd_success) {
    // NOLINTNEXTLINE(misc-include-cleaner)
    mtx_destroy(&queue->mutex);
    // NOLINTNEXTLINE(misc-include-cleaner)
    cnd_destroy(&queue->push_condvar);
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
  }
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (cnd_init(&queue->pop_condvar) != thrd_success) {
    // NOLINTNEXTLINE(misc-include-cleaner)
    mtx_destroy(&queue->mutex);
    // NOLINTNEXTLINE(misc-include-cleaner)
    cnd_destroy(&queue->pop_condvar);
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
  }
  if (!(queue->queue = av_fifo_alloc2(initial_capacity, sizeof(AVPacket*),
                                      AV_FIFO_FLAG_AUTO_GROW))) {
    // NOLINTNEXTLINE(misc-include-cleaner)
    mtx_destroy(&queue->mutex);
    // NOLINTNEXTLINE(misc-include-cleaner)
    cnd_destroy(&queue->push_condvar);
    // NOLINTNEXTLINE(misc-include-cleaner)
    cnd_destroy(&queue->pop_condvar);
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
  }
  return sve4_decode_success;
}

sve4_decode_error_t sve4_ffmpeg_packet_queue_push(
    sve4_ffmpeg_packet_queue_t* _Nonnull queue, AVPacket* _Nullable packet,
    const struct timespec* time_point, bool force_push) {
  sve4_decode_error_t err;
  // NOLINTNEXTLINE(misc-include-cleaner)
  err = thread_err_to_sve4(time_point ? mtx_timedlock(&queue->mutex, time_point)
                                      // NOLINTNEXTLINE(misc-include-cleaner)
                                      : mtx_lock(&queue->mutex));
  if (!sve4_decode_error_is_success(err))
    return err;

  // Wait until there is space
  while (!force_push && av_fifo_can_write(queue->queue) == 0) {
    sve4_log_debug("ffmpeg: packet queue full, waiting for packets to be "
                   "popped... (condvar %p)",
                   (void*)&queue->push_condvar);
    err = thread_err_to_sve4(
        time_point
            // NOLINTNEXTLINE(misc-include-cleaner)
            ? cnd_timedwait(&queue->push_condvar, &queue->mutex, time_point)
            // NOLINTNEXTLINE(misc-include-cleaner)
            : cnd_wait(&queue->push_condvar, &queue->mutex));
    if (!sve4_decode_error_is_success(err))
      goto fail;
  }

  // Push pointer to FIFO
  sve4_log_debug("ffmpeg: pushing packet %p to queue", (void*)packet);
  if (av_fifo_write(queue->queue, &packet, 1) < 0) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }

  // Wake any thread waiting on pop
  sve4_log_debug("ffmpeg: signaling pop condvar %p after pushing packet",
                 (void*)&queue->pop_condvar);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (cnd_signal(&queue->pop_condvar) != thrd_success)
    sve4_log_error("Failed to signal condition variable after pushing packet");

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_unlock(&queue->mutex) != thrd_success)
    sve4_log_error("Failed to unlock mutex after pushing packet");

  return sve4_decode_success;
fail:
  if (mtx_unlock(&queue->mutex) != thrd_success)
    sve4_log_error("Failed to unlock mutex after pushing packet");

  return err;
}

sve4_decode_error_t
sve4_ffmpeg_packet_queue_pop(sve4_ffmpeg_packet_queue_t* _Nonnull queue,
                             AVPacket* _Nullable* _Nonnull packet,
                             const struct timespec* time_point) {
  sve4_decode_error_t err = sve4_decode_success;
  // NOLINTNEXTLINE(misc-include-cleaner)
  err = thread_err_to_sve4(time_point ? mtx_timedlock(&queue->mutex, time_point)
                                      : mtx_lock(&queue->mutex));
  if (!sve4_decode_error_is_success(err))
    return err;

  // Wait until there is space
  size_t can_read = 0;
  while ((can_read = av_fifo_can_read(queue->queue)) == 0) {
    sve4_log_debug("ffmpeg: packet queue empty, waiting for packets to be "
                   "pushed... (condvar %p)",
                   (void*)&queue->pop_condvar);
    err = thread_err_to_sve4(
        time_point
            // NOLINTNEXTLINE(misc-include-cleaner)
            ? cnd_timedwait(&queue->pop_condvar, &queue->mutex, time_point)
            // NOLINTNEXTLINE(misc-include-cleaner)
            : cnd_wait(&queue->pop_condvar, &queue->mutex));
    if (!sve4_decode_error_is_success(err))
      goto fail;
  }
  (void)can_read;

  sve4_log_debug("ffmpeg: packet arrived, popping packet from queue");
  // Push pointer to FIFO
  err = sve4_decode_ffmpegerr(av_fifo_read(queue->queue, packet, 1));
  if (!sve4_decode_error_is_success(err))
    goto fail;

  // Wake any thread waiting on push
  sve4_log_debug("ffmpeg: signaling push condvar %p after popping packet",
                 (void*)&queue->push_condvar);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (cnd_signal(&queue->push_condvar) != thrd_success)
    sve4_log_error("Failed to signal condition variable after popping packet");

  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_unlock(&queue->mutex) != thrd_success)
    sve4_log_error("Failed to unlock mutex after popping packet");

  return sve4_decode_success;
fail:
  if (mtx_unlock(&queue->mutex) != thrd_success)
    sve4_log_error("Failed to unlock mutex after popping packet");

  return err;
}

sve4_decode_error_t
sve4_ffmpeg_packet_queue_is_empty(sve4_ffmpeg_packet_queue_t* _Nonnull queue,
                                  bool* _Nonnull is_empty) {
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (mtx_lock(&queue->mutex) != thrd_success)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);

  *is_empty = av_fifo_can_read(queue->queue) == 0;

  if (mtx_unlock(&queue->mutex) != thrd_success)
    sve4_log_error("Failed to unlock mutex after getting packet queue status");

  return sve4_decode_success;
}

sve4_decode_error_t
sve4_ffmpeg_packet_queue_clear(sve4_ffmpeg_packet_queue_t* _Nonnull queue) {
  if (!queue->queue)
    return sve4_decode_success;
  if (mtx_lock(&queue->mutex) != thrd_success)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_THREADS);
  AVPacket* pkt = NULL;
  while (av_fifo_can_read(queue->queue) >= 1) {
    int err = av_fifo_read(queue->queue, &pkt, 1);
    if (err < 0) {
      sve4_log_error("Failed to read packet from FIFO during free: %d", err);
      break;
    }
    av_packet_free(&pkt);
  }
  if (mtx_unlock(&queue->mutex) != thrd_success)
    sve4_log_error("Failed to unlock mutex after getting packet queue status");
  return sve4_decode_success;
}

void sve4_ffmpeg_packet_queue_free(sve4_ffmpeg_packet_queue_t* _Nonnull queue) {
  if (!queue->queue)
    return;
  sve4_ffmpeg_packet_queue_clear(queue);
  av_fifo_freep2(&queue->queue);
  cnd_destroy(&queue->push_condvar);
  cnd_destroy(&queue->pop_condvar);
  mtx_destroy(&queue->mutex);
}
