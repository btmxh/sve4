#include "ffmpeg_decoder.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "libsve4_decode/ffmpeg_demuxer.h"
#include "libsve4_decode/frame.h"
#include "libsve4_decode/ram_frame.h"
#include "libsve4_log/api.h"
#include "libsve4_utils/allocator.h"
#include "libsve4_utils/buffer.h"
#include "libsve4_utils/formats.h"

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>

#include "error.h"
#include "ffmpeg_packet_queue.h"

sve4_decode_error_t sve4_decode_ffmpeg_decoder_inner_init_packet_queue(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder) {
  sve4_log_debug("ffmpeg: initializing packet queue for decoder %p",
                 (void*)decoder);
  return sve4_ffmpeg_packet_queue_init(&decoder->packet_queue,
                                       decoder->packet_queue_initial_capacity);
}

// buffer is structured like this, but this is not standard:
// typedef struct {
//   sve4_decode_ram_frame_t frame;
//   AVFrame* av_frame;
// } av_ram_frame_t;

static void av_frame_destructor(char* mem) {
  AVFrame* av_frame = NULL;
  memcpy(&av_frame, mem + sizeof(sve4_decode_ram_frame_t), sizeof(av_frame));
  sve4_log_debug("ffmpeg: freeing AVFrame %p backing ram frame",
                 (void*)av_frame);
  av_frame_free(&av_frame);
}

static void convert_pts(AVFrame* frame, int64_t orig_time_base_num,
                        int64_t orig_time_base_den) {
  // basically
  // frame->pts = av_rescale_q(frame->pts, orig_time_base,
  //                           (AVRational){1, SVE2_NS_PER_SEC});
  // frame->duration = av_rescale_q(frame->duration, orig_time_base,
  //                                (AVRational){1, SVE2_NS_PER_SEC});
  // but without overflowing
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  int64_t num = orig_time_base_num * (int64_t)1e9;
  int64_t den = orig_time_base_den;
  frame->pts = av_rescale(frame->pts, num, den);
  frame->duration = av_rescale(frame->duration, num, den);
}

static sve4_decode_error_t map_frame_to_sve4_frame(
    AVFrame* _Nonnull av_frame, AVCodecContext* _Nonnull ctx,
    sve4_decode_frame_t* _Nonnull frame, sve4_allocator_t* frame_allocator) {
  sve4_decode_frame_free(frame);
  convert_pts(av_frame, ctx->time_base.num, ctx->time_base.den);

  frame->kind = SVE4_DECODE_FRAME_KIND_RAM_FRAME;
  frame->format = (sve4_fmt_t){
      .kind = SVE4_PIXFMT,
      .pixfmt = sve4_pixfmt_canonicalize((sve4_pixfmt_t){
          .source = SVE4_FMT_SRC_FFMPEG,
          .pixfmt = av_frame->format,
      }),
  };
  frame->pts = av_frame->pts;
  frame->duration = av_frame->duration;
  frame->width = (size_t)av_frame->width;
  frame->height = (size_t)av_frame->height;

  frame->buffer = sve4_buffer_create(
      frame_allocator, sizeof(sve4_decode_ram_frame_t) + sizeof(AVFrame*),
      av_frame_destructor);
  if (!frame->buffer)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  char* mem = sve4_buffer_get_data(frame->buffer);
#pragma GCC diagnostic pop
  memcpy(mem + sizeof(sve4_decode_ram_frame_t), &av_frame, sizeof(AVFrame*));

  sve4_decode_ram_frame_t* ram_frame = (sve4_decode_ram_frame_t*)(void*)mem;
  for (size_t i = 0; i < SVE4_DECODE_RAM_FRAME_MAX_PLANES; ++i) {
    ram_frame->data[i] = av_frame->data[i];
    ram_frame->linesizes[i] = (size_t)av_frame->linesize[i];
  }

  return sve4_decode_success;
}

sve4_decode_error_t sve4_decode_ffmpeg_decoder_inner_get_frame(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder,
    sve4_decode_frame_t* _Nonnull frame,
    const struct timespec* _Nullable deadline) {
  sve4_decode_error_t err;
  AVFrame* av_frame = av_frame_alloc();
  while (true) {
    err = sve4_decode_ffmpegerr(avcodec_receive_frame(decoder->ctx, av_frame));
    // NOLINTNEXTLINE(misc-include-cleaner)
    if (err.error_code == AVERROR(EAGAIN)) {
      sve4_log_debug("ffmpeg: decoder %p needs more packets to produce a frame",
                     (void*)decoder);
      // read a packet
      AVPacket* packet = NULL;
      err = sve4_decode_ffmpeg_demuxer_read_packet(decoder->demuxer, decoder,
                                                   &packet, deadline);
      if (!sve4_decode_error_is_success(err))
        goto fail;

      sve4_log_debug("ffmpeg: decoder %p read packet %p from demuxer",
                     (void*)decoder, (void*)packet);
      err = sve4_decode_ffmpegerr(avcodec_send_packet(decoder->ctx, packet));
      av_packet_free(&packet);
      if (!sve4_decode_error_is_success(err))
        goto fail;

      continue;
    }

    if (!sve4_decode_error_is_success(err))
      goto fail;

    sve4_log_debug("ffmpeg: decoder %p produced AVFrame* %p", (void*)decoder,
                   (void*)av_frame);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
    err = map_frame_to_sve4_frame(av_frame, decoder->ctx, frame,
                                  decoder->frame_allocator);
#pragma GCC diagnostic pop
    if (!sve4_decode_error_is_success(err))
      goto fail;
    sve4_log_debug("ffmpeg: mapped AVFrame* %p to sve4_decode_frame_t %p",
                   (void*)av_frame, (void*)frame);
    return sve4_decode_success;
  }

fail:
  av_frame_free(&av_frame);
  return err;
}

sve4_decode_error_t sve4_decode_ffmpeg_decoder_inner_seek(
    sve4_decode_ffmpeg_decoder_t* _Nonnull decoder, int64_t pos) {
  return sve4_decode_ffmpeg_demuxer_seek(decoder->demuxer, pos);
}
