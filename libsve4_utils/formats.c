#include "formats.h"

#include <stddef.h>
#include <stdio.h>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <stdlib.h>

// sve4_log is not available here
#define sve4_panic(...)                                                        \
  do {                                                                         \
    printf(__VA_ARGS__);                                                       \
    exit(1);                                                                   \
  } while (1);

#ifdef SVE4_UTILS_HAVE_FFMPEG
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#endif

#include "allocator.h"

const char* _Nonnull sve4_pixfmt_to_string(sve4_pixfmt_t pixfmt) {
  switch (pixfmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    switch ((sve4_pixfmt_default_t)pixfmt.pixfmt) {
    case SVE4_PIXFMT_DEFAULT_UNKNOWN:
      break;
    case SVE4_PIXFMT_DEFAULT_RGBA8:
      return "rgba";
    case SVE4_PIXFMT_DEFAULT_ARGB8:
      return "argb";
    }
    break;
  case SVE4_FMT_SRC_FFMPEG:
#ifdef SVE4_UTILS_HAVE_FFMPEG
    return av_get_pix_fmt_name((enum AVPixelFormat)pixfmt.pixfmt);
#else
    sve4_panic("FFmpeg pixfmt used but libsve4_utils is not compiled with "
               "FFmpeg support");
#endif
    break;
  }
  return "unknown";
}

const char* _Nonnull sve4_samplefmt_to_string(sve4_samplefmt_t samplefmt) {
  switch (samplefmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    switch ((sve4_samplefmt_default_t)samplefmt.format) {
    case SVE4_SAMPLEFMT_DEFAULT_UNKNOWN:
      break;
    case SVE4_SAMPLEFMT_DEFAULT_S16:
      return "s16";
    }
    break;
  case SVE4_FMT_SRC_FFMPEG:
#ifdef SVE4_UTILS_HAVE_FFMPEG
    return av_get_sample_fmt_name((enum AVSampleFormat)samplefmt.format);
#else
    sve4_panic("FFmpeg samplefmt used but libsve4_utils is not compiled with "
               "FFmpeg support");
#endif
    break;
  }
  return "unknown";
}

const char* _Nonnull sve4_genericfmt_to_string(sve4_genericfmt_t genericfmt) {
  switch (genericfmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    switch ((sve4_genericfmt_default_t)genericfmt.format) {
    case SVE4_GENERICFMT_DEFAULT_UNKNOWN:
      break;
    }
    break;
  case SVE4_FMT_SRC_FFMPEG:
    break;
  }
  return "unknown";
}

size_t sve4_pixfmt_num_planes(sve4_pixfmt_t pixfmt) {
  switch (pixfmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    switch ((sve4_pixfmt_default_t)pixfmt.pixfmt) {
    case SVE4_PIXFMT_DEFAULT_UNKNOWN:
      return 0;
    case SVE4_PIXFMT_DEFAULT_RGBA8:
    case SVE4_PIXFMT_DEFAULT_ARGB8:
      return 1;
    }
  case SVE4_FMT_SRC_FFMPEG:
#ifdef SVE4_UTILS_HAVE_FFMPEG
    return (size_t)av_pix_fmt_count_planes(pixfmt.pixfmt);
#else
    sve4_panic("FFmpeg pixfmt used but libsve4_utils is not compiled with "
               "FFmpeg support");
#endif
  }

  return 0;
}

size_t sve4_pixfmt_linesize(sve4_pixfmt_t pixfmt, size_t plane, size_t width,
                            size_t align) {
  switch (pixfmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    switch ((sve4_pixfmt_default_t)pixfmt.pixfmt) {
    case SVE4_PIXFMT_DEFAULT_UNKNOWN:
      return 0;
    case SVE4_PIXFMT_DEFAULT_RGBA8:
    case SVE4_PIXFMT_DEFAULT_ARGB8:
      return plane ? 0 : sve4_align_up(width * 4, align);
    }
  case SVE4_FMT_SRC_FFMPEG:
#ifdef SVE4_UTILS_HAVE_FFMPEG
    return sve4_align_up(
        (size_t)av_image_get_linesize(pixfmt.pixfmt, (int)plane, (int)width),
        align);
#else
    sve4_panic("FFmpeg pixfmt used but libsve4_utils is not compiled with "
               "FFmpeg support");
#endif
  }

  return 0;
}

sve4_pixfmt_t sve4_pixfmt_canonicalize(sve4_pixfmt_t pixfmt) {
  switch (pixfmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    return pixfmt;
  case SVE4_FMT_SRC_FFMPEG:
#ifdef SVE4_UTILS_HAVE_FFMPEG
    switch (pixfmt.pixfmt) {
    case AV_PIX_FMT_RGBA:
      return sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8);
    case AV_PIX_FMT_ARGB:
      return sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8);
    default:;
    }
#else
    sve4_panic("FFmpeg pixfmt used but libsve4_utils is not compiled with "
               "FFmpeg support");
#endif
    break;
  }
  return pixfmt;
}

bool sve4_pixfmt_eq(sve4_pixfmt_t lhs, sve4_pixfmt_t rhs) {
  lhs = sve4_pixfmt_canonicalize(lhs);
  rhs = sve4_pixfmt_canonicalize(rhs);
  return lhs.pixfmt == rhs.pixfmt && lhs.source == rhs.source;
}
