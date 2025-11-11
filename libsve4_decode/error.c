#include "error.h"

#ifdef SVE4_DECODE_HAVE_FFMPEG
#include <libavutil/error.h>
#endif
sve4_decode_error_t sve4_decode_error_canonicalize(sve4_decode_error_t err) {
  switch (err.source) {
  case SVE4_DECODE_ERROR_SRC_DEFAULT:
    return err;
#ifdef SVE4_DECODE_HAVE_FFMPEG
  case SVE4_DECODE_ERROR_SRC_FFMPEG:
    switch (err.error_code) {
    case 0:
      return sve4_decode_success;
    case AVERROR_EOF:
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_EOF);
    default:;
    }
    break;
#endif
  default:;
  }
  return err;
}
