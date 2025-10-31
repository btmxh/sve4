#include "formats.h"

#include <stddef.h>

#include "allocator.h"

const char* _Nonnull sve4_pixfmt_to_string(sve4_pixfmt_t pixfmt) {
  switch (pixfmt.source) {
  case SVE4_FMT_SRC_DEFAULT:
    switch ((sve4_pixfmt_default_t)pixfmt.pixfmt) {
    case SVE4_PIXFMT_DEFAULT_UNKNOWN:
      break;
    case SVE4_PIXFMT_DEFAULT_RGBA8:
      return "SVE4_RGBA8";
    }
  case SVE4_FMT_SRC_FFMPEG:
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
      return "SVE4_S16";
    }
  case SVE4_FMT_SRC_FFMPEG:
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
      return 1;
    }
  case SVE4_FMT_SRC_FFMPEG:
    break;
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
      return plane ? 0 : sve4_align_up(width * 4, align);
    }
  case SVE4_FMT_SRC_FFMPEG:
    break;
  }

  return 0;
}
