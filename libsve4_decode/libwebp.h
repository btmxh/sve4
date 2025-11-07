#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sve4_decode_export.h"

#include <libsve4_utils/allocator.h>
#include <libsve4_utils/defines.h>
#include <webp/demux.h>

#include "error.h"
#include "frame.h"

typedef struct SVE4_DECODE_EXPORT {
  WebPData data;
  WebPAnimDecoder* _Nullable decoder;
  size_t width, height, num_frames;
  int64_t last_pts;
} sve4_decode_libwebp_anim_t;

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_libwebp_open_anim(sve4_decode_libwebp_anim_t* _Nonnull anim,
                              const uint8_t* _Nonnull data, size_t data_size);
SVE4_DECODE_EXPORT
void sve4_decode_libwebp_close_anim(sve4_decode_libwebp_anim_t* _Nullable anim);

SVE4_DECODE_EXPORT
size_t
sve4_decode_libwebp_anim_get_width(sve4_decode_libwebp_anim_t* _Nonnull anim);
SVE4_DECODE_EXPORT
size_t
sve4_decode_libwebp_anim_get_height(sve4_decode_libwebp_anim_t* _Nonnull anim);
SVE4_DECODE_EXPORT
size_t sve4_decode_libwebp_anim_get_num_frames(
    sve4_decode_libwebp_anim_t* _Nonnull anim);
SVE4_DECODE_EXPORT
void sve4_decode_libwebp_anim_reset(sve4_decode_libwebp_anim_t* _Nullable anim);
SVE4_DECODE_EXPORT
bool sve4_decode_libwebp_anim_has_more(
    sve4_decode_libwebp_anim_t* _Nullable anim);
SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_libwebp_anim_alloc(sve4_decode_libwebp_anim_t* _Nonnull anim,
                               sve4_allocator_t* _Nullable allocator,
                               sve4_decode_frame_t* _Nonnull frame);
SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_libwebp_anim_decode(sve4_decode_libwebp_anim_t* _Nonnull anim,
                                sve4_decode_frame_t* _Nullable frame);

typedef struct SVE4_DECODE_EXPORT {
  WebPData data;
  WebPDemuxer* _Nullable demuxer;
  WebPIterator iterator;
} sve4_decode_libwebp_demux_t;

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_libwebp_open_demux(sve4_decode_libwebp_demux_t* _Nonnull demux,
                               const uint8_t* _Nonnull data, size_t data_size);
SVE4_DECODE_EXPORT
void sve4_decode_libwebp_close_demux(
    sve4_decode_libwebp_demux_t* _Nullable demux);

SVE4_DECODE_EXPORT
size_t sve4_decode_libwebp_demux_get_width(
    sve4_decode_libwebp_demux_t* _Nonnull demux);
SVE4_DECODE_EXPORT
size_t sve4_decode_libwebp_demux_get_height(
    sve4_decode_libwebp_demux_t* _Nonnull demux);
SVE4_DECODE_EXPORT
size_t sve4_decode_libwebp_demux_get_num_frames(
    sve4_decode_libwebp_demux_t* _Nonnull demux);
SVE4_DECODE_EXPORT
int64_t sve4_decode_libwebp_demux_get_total_duration(
    sve4_decode_libwebp_demux_t* _Nonnull demux);
