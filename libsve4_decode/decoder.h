#pragma once

#include <stdint.h>
#include <time.h>

#include "sve4_decode_export.h"

#include "libsve4_decode/error.h"
#include "libsve4_decode/frame.h"
#include "libsve4_utils/buffer.h"

#ifdef SVE4_DECODE_HAVE_FFMPEG
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#endif

typedef enum {
  SVE4_DECODE_DECODER_BACKEND_AUTO = 0,
  SVE4_DECODE_DECODER_BACKEND_LIBWEBP,
  SVE4_DECODE_DECODER_BACKEND_FFMPEG,
} sve4_decode_decoder_backend_t;

typedef struct sve4_decode_decoder_t {
  sve4_decode_decoder_backend_t backend;
  sve4_buffer_ref_t _Nullable data;
  sve4_buffer_ref_t _Nullable demuxer; // not owning!
  sve4_decode_error_t (*_Nullable get_frame)(
      struct sve4_decode_decoder_t* _Nonnull decoder,
      sve4_decode_frame_t* _Nullable frame,
      const struct timespec* _Nullable deadline);
  sve4_decode_error_t (*_Nullable seek)(
      struct sve4_decode_decoder_t* _Nonnull decoder, int64_t pos);
} sve4_decode_decoder_t;

typedef enum {
  SVE4_DECODE_MEDIA_TYPE_UNKNOWN,
  SVE4_DECODE_MEDIA_TYPE_VIDEO,
  SVE4_DECODE_MEDIA_TYPE_AUDIO,
  SVE4_DECODE_MEDIA_TYPE_SUBTITLE,
} sve4_decode_media_type_t;

typedef struct {
  const char* _Nullable title;
  const char* _Nullable language;
  sve4_decode_media_type_t type;
  bool is_forced : 1;
  bool is_default : 1;

  void* _Nullable opaque;
} sve4_decode_stream_t;

typedef struct sve4_decode_stream_chooser_t {
  union {
    sve4_buffer_ref_t _Nullable user_data;
    struct {
      sve4_decode_media_type_t type : 2;
      uint16_t offset;
    } media_type_data;
  };
  size_t (*_Nullable get_stream)(
      sve4_decode_decoder_t* _Nonnull decoder,
      const struct sve4_decode_stream_chooser_t* _Nonnull chooser,
      sve4_decode_stream_t* _Nonnull streams, size_t nb_streams);
} sve4_decode_stream_chooser_t;

SVE4_DECODE_EXPORT
sve4_decode_stream_chooser_t
sve4_decode_stream_chooser_typed(sve4_decode_media_type_t type,
                                 uint16_t offset);

size_t sve4_decode_stream_choose(
    sve4_decode_decoder_t* _Nonnull decoder,
    const struct sve4_decode_stream_chooser_t* _Nonnull chooser,
    sve4_decode_stream_t* _Nonnull streams, size_t nb_streams);

typedef struct {
  const char* _Nonnull url;
  sve4_decode_decoder_backend_t backend;
  sve4_allocator_t* _Nullable allocator;
  sve4_allocator_t* _Nullable frame_allocator;
  sve4_decode_stream_chooser_t stream_chooser;
  sve4_buffer_ref_t _Nullable demuxer;
  size_t packet_queue_initial_capacity; // only useful for multi-decoder setups

  struct {
#ifdef SVE4_DECODE_HAVE_FFMPEG
    const AVInputFormat* _Nullable fmt;
    AVDictionary* _Nullable* _Nullable options;
#else
    int dummy;
#endif
  }* _Nullable avformat_open_input;

  struct {
#ifdef SVE4_DECODE_HAVE_FFMPEG
    AVDictionary* _Nullable* _Nullable options;
#else
    int dummy;
#endif
  }* _Nullable avformat_find_stream_info;

  struct {
#ifdef SVE4_DECODE_HAVE_FFMPEG
    const AVCodec* _Nullable codec;
    AVDictionary* _Nullable* _Nullable options;
#else
    int dummy;
#endif
  }* _Nullable avcodec_open2;

  struct {
#ifdef SVE4_DECODE_HAVE_FFMPEG
    void* _Nullable user_ptr;
    void (*_Nullable setup)(AVCodecContext* _Nonnull codec_ctx,
                            void* _Nullable user_ptr);
#else
    int dummy;
#endif
  }* _Nullable setup_codec_context;
} sve4_decode_decoder_config_t;

SVE4_DECODE_EXPORT
sve4_decode_decoder_backend_t
sve4_decode_select_backend(const sve4_decode_decoder_config_t* _Nonnull config);

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_decoder_open(sve4_decode_decoder_t* _Nonnull decoder,
                         const sve4_decode_decoder_config_t* _Nonnull config);

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_decoder_get_frame(sve4_decode_decoder_t* _Nonnull decoder,
                              sve4_decode_frame_t* _Nullable frame,
                              const struct timespec* _Nullable deadline);

SVE4_DECODE_EXPORT
sve4_decode_error_t
sve4_decode_decoder_seek(sve4_decode_decoder_t* _Nonnull decoder, int64_t pos);

SVE4_DECODE_EXPORT
void sve4_decode_decoder_close(sve4_decode_decoder_t* _Nullable decoder);

SVE4_DECODE_EXPORT
sve4_buffer_ref_t _Nullable sve4_decode_decoder_get_demuxer(
    sve4_decode_decoder_t* _Nullable decoder);
