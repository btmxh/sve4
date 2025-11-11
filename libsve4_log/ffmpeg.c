#include "ffmpeg.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "libsve4_log/api.h"
#include "libsve4_utils/allocator.h"
#include "libsve4_utils/arena.h"

#include <libavutil/fifo.h>
#include <libavutil/log.h>
#include <tinycthread.h>

static sve4_log_level_t ffmpeg_level_to_sve4(int level) {
  switch (level) {
  case AV_LOG_TRACE:
  case AV_LOG_DEBUG:
    return SVE4_LOG_LEVEL_DEBUG;
  case AV_LOG_INFO:
    return SVE4_LOG_LEVEL_INFO;
  case AV_LOG_WARNING:
    return SVE4_LOG_LEVEL_WARNING;
  case AV_LOG_ERROR:
    return SVE4_LOG_LEVEL_ERROR;
  default:
    return SVE4_LOG_LEVEL_INFO;
  }
}

// here, FFmpeg does not log its stuff in separated messages, but it can log a
// message in multiple log calls like so:
// ```c
// sve3_ffmpeg_log_callback(..., "Hello, ");
// sve3_ffmpeg_log_callback(..., "world!\n");
// ````
//
// Hence, we need to manually group these messages into multiple lines.
void sve4_log_ffmpeg_callback(void* avcl, int level, const char* fmt,
                              va_list args) {
  (void)avcl;
  // Our approach uses an AVFifo object for every thread
  static _Thread_local AVFifo* msg = NULL;
  // along with a temporary buffer managed in an arena
  static _Thread_local sve4_allocator_t arena;

  // lazily initialize the AVFifo
  if (!msg) {
    msg = av_fifo_alloc2(0, sizeof(char), AV_FIFO_FLAG_AUTO_GROW);
    arena = sve4_allocator_arena_init;
  }

  va_list args2;
  va_copy(args2, args);

  // first pass: use vsnprintf to figure out how much memory should be
  // reserved...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  int len = vsnprintf(NULL, 0, fmt, args2);
#pragma GCC diagnostic pop
  va_end(args2);
  assert(len >= 0);

  // then allocate the temporary buffer for the message
  char* data = sve4_malloc(&arena, (size_t)(len + 1));
  if (!data) {
    sve4_panic("sve4_malloc failed in sve4_log_ffmpeg_callback");
  }

  // second pass: actually format the message
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  vsnprintf(data, (size_t)(len + 1), fmt, args);
#pragma GCC diagnostic pop
  // vsnprintf will add a handy \0 at the end of the message, but we will just
  // ignore it

  // now, split the message based on newline characters
  // memchr is possible but here we use strchr to simplify things
  for (char* ptr = strchr(data, '\n'); ptr;
       data = ptr + 1, ptr = strchr(data, '\n')) {
    // handle the case where
    // add the rest of the line (from data to ptr + 1) to the fifo
    av_fifo_write(msg, data, (size_t)(ptr + 1 - data));

    size_t msg_fmt_len = av_fifo_can_read(msg);
    char* msg_fmt = sve4_malloc(&arena, msg_fmt_len + 1);
    if (!msg_fmt) {
      sve4_panic("sve4_malloc failed in sve4_log_ffmpeg_callback");
    }
    av_fifo_read(msg, msg_fmt, msg_fmt_len);
    msg_fmt[msg_fmt_len] = '\0';
    sve4_glog(SVE4_LOG_ID_DEFAULT_FFMPEG, __FILE__, __LINE__, false,
              ffmpeg_level_to_sve4(level), "%.*s", (int)msg_fmt_len, msg_fmt);
  }

  // write the rest in the fifo
  av_fifo_write(msg, data, strlen(data));

  // free temp buffer, in theory this can leak but whatever...
  sve4_allocator_arena_reset(&arena);
}
