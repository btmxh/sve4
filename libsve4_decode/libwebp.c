#include "libwebp.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <webp/demux.h>

#include "allocator.h"
#include "buffer.h"
#include "error.h"
#include "formats.h"
#include "frame.h"
#include "ram_frame.h"

enum { ms_to_ns = (int64_t)1e6 };

sve4_decode_error_t
sve4_decode_libwebp_open_anim(sve4_decode_libwebp_anim_t* anim,
                              const uint8_t* data, size_t data_size) {
  assert(anim);
  anim->data.bytes = data;
  anim->data.size = data_size;

  anim->decoder = WebPAnimDecoderNew(&anim->data, NULL);
  if (!anim->decoder) {
    sve4_decode_libwebp_close_anim(anim);
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_GENERIC);
  }

  WebPAnimInfo info;
  if (!WebPAnimDecoderGetInfo(anim->decoder, &info)) {
    sve4_decode_libwebp_close_anim(anim);
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_GENERIC);
  }

  anim->width = info.canvas_width;
  anim->height = info.canvas_height;
  anim->num_frames = info.frame_count;
  anim->last_pts = 0;
  return sve4_decode_success;
}

void sve4_decode_libwebp_close_anim(sve4_decode_libwebp_anim_t* anim) {
  if (!anim)
    return;
  WebPAnimDecoderDelete(anim->decoder);
  memset(anim, 0, sizeof *anim);
}

size_t
sve4_decode_libwebp_anim_get_width(sve4_decode_libwebp_anim_t* _Nonnull anim) {
  assert(anim);
  return anim->width;
}

size_t
sve4_decode_libwebp_anim_get_height(sve4_decode_libwebp_anim_t* _Nonnull anim) {
  assert(anim);
  return anim->height;
}

size_t sve4_decode_libwebp_anim_get_num_frames(
    sve4_decode_libwebp_anim_t* _Nonnull anim) {
  assert(anim);
  return anim->num_frames;
}

void sve4_decode_libwebp_anim_reset(
    sve4_decode_libwebp_anim_t* _Nullable anim) {
  if (anim && anim->decoder)
    WebPAnimDecoderReset(anim->decoder);
}

bool sve4_decode_libwebp_anim_has_more(
    sve4_decode_libwebp_anim_t* _Nullable anim) {
  return anim && anim->decoder && WebPAnimDecoderHasMoreFrames(anim->decoder);
}

sve4_decode_error_t
sve4_decode_libwebp_anim_alloc(sve4_decode_libwebp_anim_t* _Nonnull anim,
                               sve4_allocator_t* _Nullable allocator,
                               sve4_decode_frame_t* _Nonnull frame) {
  return sve4_decode_alloc_ram_frame(
      frame, allocator, sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8),
      anim->width, anim->height, (size_t[]){1});
}

sve4_decode_error_t
sve4_decode_libwebp_anim_decode(sve4_decode_libwebp_anim_t* _Nonnull anim,
                                sve4_decode_frame_t* frame) {
  uint8_t* buf = NULL;
  int timestamp = 0;
  if (!WebPAnimDecoderGetNext(anim->decoder, &buf, &timestamp)) {
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_GENERIC);
  }

  if (frame) {
    assert(frame->buffer && frame->kind == SVE4_DECODE_FRAME_KIND_RAM_FRAME);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullable-to-nonnull-conversion"
    sve4_decode_ram_frame_t* ram_frame = sve4_buffer_get_data(frame->buffer);
#pragma clang diagnostic pop

    // buf is in RGBA8
    memcpy(ram_frame->data[0], buf, ram_frame->linesizes[0] * anim->height);
    int64_t pts = timestamp * ms_to_ns;
    frame->pts = anim->last_pts;
    frame->duration = pts - anim->last_pts;
    anim->last_pts = pts;
  }
  return sve4_decode_success;
}

sve4_decode_error_t
sve4_decode_libwebp_open_demux(sve4_decode_libwebp_demux_t* _Nonnull demux,
                               const uint8_t* _Nonnull data, size_t data_size) {
  assert(demux);
  demux->data.bytes = data;
  demux->data.size = data_size;

  demux->demuxer = WebPDemux(&demux->data);
  if (!demux->demuxer) {
    sve4_decode_libwebp_close_demux(demux);
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_GENERIC);
  }

  return sve4_decode_success;
}

void sve4_decode_libwebp_close_demux(sve4_decode_libwebp_demux_t* demux) {
  if (!demux)
    return;
  WebPDemuxDelete(demux->demuxer);
}
size_t sve4_decode_libwebp_demux_get_width(
    sve4_decode_libwebp_demux_t* _Nonnull demux) {
  return WebPDemuxGetI(demux->demuxer, WEBP_FF_CANVAS_WIDTH);
}

size_t sve4_decode_libwebp_demux_get_height(
    sve4_decode_libwebp_demux_t* _Nonnull demux) {
  return WebPDemuxGetI(demux->demuxer, WEBP_FF_CANVAS_HEIGHT);
}

size_t sve4_decode_libwebp_demux_get_num_frames(
    sve4_decode_libwebp_demux_t* _Nonnull demux) {
  return WebPDemuxGetI(demux->demuxer, WEBP_FF_FRAME_COUNT);
}

int64_t sve4_decode_libwebp_demux_get_total_duration(
    sve4_decode_libwebp_demux_t* _Nonnull demux) {
  int64_t duration = 0;
  WebPIterator iter;
  if (WebPDemuxGetFrame(demux->demuxer, 1, &iter)) {
    do {
      duration += iter.duration * ms_to_ns;
    } while (WebPDemuxNextFrame(&iter));
    return duration;
  }

  return -1;
}
