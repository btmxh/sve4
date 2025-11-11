#include "libwebp.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libsve4_decode/read.h"
#include "libsve4_log/api.h"

#include <libsve4_utils/allocator.h>
#include <libsve4_utils/buffer.h>
#include <libsve4_utils/formats.h>
#include <webp/demux.h>

#include "decoder.h"
#include "error.h"
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

  sve4_log_debug("libwebp: opened AnimDecoder with %zu frames, size %zux%zu",
                 anim->num_frames, anim->width, anim->height);

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
      anim->width, anim->height, (const size_t[]){1});
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
    sve4_decode_ram_frame_t* ram_frame = sve4_buffer_get_data(frame->buffer);
#pragma GCC diagnostic pop

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

typedef struct {
  sve4_decode_libwebp_anim_t anim;
  sve4_allocator_t* allocator;
  sve4_allocator_t* frame_allocator;
  void* ptr;
} decoder_inner_t;

static void decoder_destructor(char* mem) {
  decoder_inner_t* inner = (decoder_inner_t*)mem;
  sve4_decode_libwebp_close_anim(&inner->anim);
  sve4_free(inner->allocator, inner->ptr);
}

static bool is_frame_compatible(sve4_decode_libwebp_anim_t* anim,
                                sve4_decode_frame_t* _Nonnull frame) {
  if (frame->kind != SVE4_DECODE_FRAME_KIND_RAM_FRAME)
    return false;
  if (frame->width != sve4_decode_libwebp_anim_get_width(anim))
    return false;
  if (frame->height != sve4_decode_libwebp_anim_get_height(anim))
    return false;
  if (frame->format.kind != SVE4_PIXFMT)
    return false;
  if (!sve4_pixfmt_eq(frame->format.pixfmt,
                      sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)))
    return false;
  return true;
}

static sve4_decode_error_t
webp_get_frame(struct sve4_decode_decoder_t* _Nonnull decoder,
               sve4_decode_frame_t* _Nullable frame,
               const struct timespec* _Nullable deadline) {
  (void)deadline;
  sve4_decode_error_t err;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  decoder_inner_t* inner =
      (decoder_inner_t*)sve4_buffer_get_data(decoder->data);
#pragma GCC diagnostic pop
  if (!sve4_decode_libwebp_anim_has_more(&inner->anim))
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_EOF);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  if (frame && !is_frame_compatible(&inner->anim, frame)) {
    sve4_log_debug(
        "libwebp: allocating new frame for decoder %p since provided frame "
        "%p is not compatible",
        (void*)decoder, (void*)frame);
    sve4_decode_frame_free(frame);
    err = sve4_decode_libwebp_anim_alloc(&inner->anim, inner->frame_allocator,
                                         frame);
    if (!sve4_decode_error_is_success(err))
      return err;
  }
#pragma GCC diagnostic pop

  return sve4_decode_libwebp_anim_decode(&inner->anim, frame);
}

static sve4_decode_error_t webp_seek(sve4_decode_decoder_t* decoder,
                                     int64_t pos) {
  (void)pos;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  decoder_inner_t* inner =
      (decoder_inner_t*)sve4_buffer_get_data(decoder->data);
#pragma GCC diagnostic pop
  // seek to the first frame
  // this is a hack since libwebp does not support seeking natively
  sve4_decode_libwebp_anim_reset(&inner->anim);
  return sve4_decode_success;
}

sve4_decode_error_t sve4_decode_libwebp_open_decoder(
    sve4_decode_decoder_t* _Nonnull decoder,
    const sve4_decode_decoder_config_t* _Nonnull config) {
  sve4_decode_error_t err;
  char* buffer = NULL;
  size_t size = SIZE_MAX;

  decoder->data = sve4_buffer_create(config->allocator, sizeof(decoder_inner_t),
                                     decoder_destructor);
  if (!decoder->data) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }
  err = sve4_decode_read_url(config->allocator, &buffer, &size, config->url,
                             true);
  if (!sve4_decode_error_is_success(err))
    goto fail;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  decoder_inner_t* inner = sve4_buffer_get_data(decoder->data);
#pragma GCC diagnostic pop
  inner->allocator = config->allocator;
  inner->frame_allocator = config->frame_allocator;
  inner->ptr = buffer;

  err =
      sve4_decode_libwebp_open_anim(&inner->anim, (const uint8_t*)buffer, size);
  if (!sve4_decode_error_is_success(err))
    goto fail;

  decoder->get_frame = webp_get_frame;
  decoder->seek = webp_seek;
  return sve4_decode_success;

fail:
  sve4_decode_decoder_close(decoder);
  return err;
}
