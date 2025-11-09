#include "decoder.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libsve4_log/api.h"
#include "libsve4_utils/buffer.h"

#include "error.h"
#include "frame.h"

#ifdef SVE4_DECODE_HAVE_WEBP
#include "libwebp.h"
#endif

#ifdef SVE4_DECODE_HAVE_FFMPEG
#include "ffmpeg.h"
#endif

static size_t
typed_get_stream(sve4_decode_decoder_t* decoder,
                 const struct sve4_decode_stream_chooser_t* _Nonnull chooser,
                 sve4_decode_stream_t* _Nonnull streams, size_t nb_streams) {
  (void)decoder;

  sve4_decode_media_type_t type = chooser->media_type_data.type;
  uint16_t offset = chooser->media_type_data.offset;
  for (size_t i = 0; i < nb_streams; i++) {
    if (streams[i].type == type) {
      if (offset-- == 0)
        return i;
    }
  }

  return SIZE_MAX;
}

static size_t
default_get_stream(sve4_decode_decoder_t* decoder,
                   const struct sve4_decode_stream_chooser_t* _Nonnull chooser,
                   sve4_decode_stream_t* _Nonnull streams, size_t nb_streams) {
  (void)decoder;
  (void)chooser;

  if (nb_streams == 0)
    return SIZE_MAX;
  for (size_t i = 0; i < nb_streams; ++i)
    if (streams[i].is_default)
      return i;
  return 0;
}

size_t sve4_decode_stream_choose(
    sve4_decode_decoder_t* _Nonnull decoder,
    const struct sve4_decode_stream_chooser_t* _Nonnull chooser,
    sve4_decode_stream_t* _Nonnull streams, size_t nb_streams) {
  return (chooser->get_stream ? chooser->get_stream : default_get_stream)(
      decoder, chooser, streams, nb_streams);
}

sve4_decode_stream_chooser_t
sve4_decode_stream_chooser_typed(sve4_decode_media_type_t type,
                                 uint16_t offset) {
  return (sve4_decode_stream_chooser_t){
      .media_type_data =
          {
              .type = type,
              .offset = offset,
          },
      .get_stream = typed_get_stream,
  };
}

static bool is_webp(const char header[], size_t size) {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  return size >= 12 && memcmp(&header[0], "RIFF", 4) == 0 &&
         memcmp(&header[8], "WEBP", 4) == 0;
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
}

sve4_decode_decoder_backend_t sve4_decode_select_backend(
    const sve4_decode_decoder_config_t* _Nonnull config) {
  sve4_log_debug("Auto-selecting decoder backend for url %s", config->url);
  if (config->backend != SVE4_DECODE_DECODER_BACKEND_AUTO)
    return config->backend;
  FILE* file = fopen(config->url, "rb");
  if (!file)
    return SVE4_DECODE_DECODER_BACKEND_FFMPEG;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  char header[64];
  size_t size = fread(header, 1, sizeof(header), file);
  fclose(file);
  sve4_log_debug("Read %zu bytes from file header for backend detection", size);

  if (is_webp(header, size)) {
    sve4_log_debug("Detected WEBP format for url %s, using LIBWEBP",
                   config->url);
    return SVE4_DECODE_DECODER_BACKEND_LIBWEBP;
  }

  sve4_log_debug("No specific format detected for url %s, using FFMPEG",
                 config->url);
  return SVE4_DECODE_DECODER_BACKEND_FFMPEG;
}

sve4_decode_error_t
sve4_decode_decoder_open(sve4_decode_decoder_t* _Nonnull decoder,
                         const sve4_decode_decoder_config_t* _Nonnull config) {
  decoder->demuxer = NULL;
  switch (decoder->backend = sve4_decode_select_backend(config)) {
  case SVE4_DECODE_DECODER_BACKEND_AUTO:
    sve4_panic("*_AUTO returned from sve4_decode_select_backend. This should "
               "not happen");
  case SVE4_DECODE_DECODER_BACKEND_LIBWEBP:
#ifdef SVE4_DECODE_HAVE_WEBP
    sve4_log_debug("Opening decoder %p using LIBWEBP backend for url %s",
                   (void*)decoder, config->url);
    return sve4_decode_libwebp_open_decoder(decoder, config);
#else
    sve4_log_warn("LIBWEBP backend selected but not available");
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_BACKEND_MISSING);
#endif

  case SVE4_DECODE_DECODER_BACKEND_FFMPEG:
#ifdef SVE4_DECODE_HAVE_FFMPEG
    sve4_log_debug("Opening decoder %p using FFMPEG backend for url %s",
                   (void*)decoder, config->url);
    return sve4_decode_ffmpeg_open_decoder(decoder, config);
#else
    sve4_log_warn("FFMPEG backend selected but not available");
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_BACKEND_MISSING);
#endif
  }

  return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_BACKEND_MISSING);
}

sve4_decode_error_t
sve4_decode_decoder_get_frame(sve4_decode_decoder_t* _Nonnull decoder,
                              sve4_decode_frame_t* _Nullable frame,
                              const struct timespec* _Nullable deadline) {
  assert(decoder);
  if (decoder->get_frame)
    return decoder->get_frame(decoder, frame, deadline);
  return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_UNIMPLEMENTED);
}

sve4_decode_error_t
sve4_decode_decoder_seek(sve4_decode_decoder_t* _Nonnull decoder, int64_t pos) {
  assert(decoder);
  sve4_log_debug("Seeking decoder %p to position %" PRId64, (void*)decoder,
                 pos);
  if (decoder->seek)
    return decoder->seek(decoder, pos);
  return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_UNIMPLEMENTED);
}

void sve4_decode_decoder_close(sve4_decode_decoder_t* decoder) {
  if (!decoder)
    return;
  sve4_log_debug("Closing decoder %p", (void*)decoder);
  sve4_buffer_free(&decoder->data);
}

sve4_buffer_ref_t
sve4_decode_decoder_get_demuxer(sve4_decode_decoder_t* _Nullable decoder) {
  return decoder ? decoder->demuxer : NULL;
}
