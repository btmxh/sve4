#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libsve4_decode/decoder.h"
#include "libsve4_decode/frame.h"
#include "libsve4_log/init_test.h"
#include "libsve4_utils/formats.h"

#include <libsve4_decode/error.h>
#include <libsve4_decode/libwebp.h>
#include <libsve4_decode/ram_frame.h>
#include <libsve4_utils/allocator.h>
#include <munit.h>

#define ASSETS_DIR "../../../../assets/"
enum { MS = (int64_t)1e6 };
#define ms *MS

static uint32_t rgba8(const uint8_t* ptr) {
  return ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) |
         ((uint32_t)ptr[2] << 8) | ((uint32_t)ptr[3] << 0);
}

static uint32_t argb8(const uint8_t* ptr) {
  return ((uint32_t)ptr[0] << 0) | ((uint32_t)ptr[1] << 24) |
         ((uint32_t)ptr[2] << 16) | ((uint32_t)ptr[3] << 8);
}

#define assert_success(err)                                                    \
  do {                                                                         \
    munit_assert_int((int)err.source, ==, SVE4_DECODE_ERROR_SRC_DEFAULT);      \
    munit_assert_int((int)err.error_code, ==,                                  \
                     SVE4_DECODE_ERROR_DEFAULT_SUCCESS);                       \
  } while (0);

static MunitResult test_simple_webp(const MunitParameter params[],
                                    void* user_data) {
  (void)user_data;

  sve4_decode_decoder_backend_t backend = SVE4_DECODE_DECODER_BACKEND_AUTO;
  for (; params->name; ++params) {
    if (strcmp(params->name, "backend") == 0) {
      if (strcmp(params->value, "AUTO") == 0) {
        backend = SVE4_DECODE_DECODER_BACKEND_AUTO;
      } else if (strcmp(params->value, "LIBWEBP") == 0) {
        backend = SVE4_DECODE_DECODER_BACKEND_LIBWEBP;
      } else if (strcmp(params->value, "FFMPEG") == 0) {
        backend = SVE4_DECODE_DECODER_BACKEND_FFMPEG;
      }
    }
  }

  const char* path = ASSETS_DIR "4x4.webp";

  sve4_decode_decoder_t decoder = {0};
  sve4_decode_error_t err;
  err = sve4_decode_decoder_open(&decoder, &(sve4_decode_decoder_config_t){
                                               .url = path,
                                               .backend = backend,
                                           });
  assert_success(err);
  sve4_decode_frame_t frame = {0};
  err = sve4_decode_decoder_get_frame(&decoder, &frame, NULL);
  assert_success(err);

  munit_assert_int((int)frame.kind, ==, SVE4_DECODE_FRAME_KIND_RAM_FRAME);
  munit_assert_size(frame.width, ==, 4);
  munit_assert_size(frame.height, ==, 4);
  munit_assert_int((int)frame.format.kind, ==, SVE4_PIXFMT);
  munit_assert_true(
      sve4_pixfmt_eq(frame.format.pixfmt,
                     sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)) ||
      sve4_pixfmt_eq(frame.format.pixfmt,
                     sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  sve4_decode_ram_frame_t* ram_frame = sve4_buffer_get_data(frame.buffer);
  const uint8_t* frame_data = ram_frame->data[0];
#pragma GCC diagnostic pop

  munit_assert_ptr_not_null(frame_data);
  uint32_t u32_frame_data[16];
  for (size_t row = 0; row < 4; ++row) {
    for (size_t col = 0; col < 4; ++col) {
      size_t pos = row * 4 + col;
      size_t ram_frame_pos = col * 4 + row * ram_frame->linesizes[0];
      if (sve4_pixfmt_eq(frame.format.pixfmt,
                         sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)))
        u32_frame_data[pos] = rgba8(&frame_data[ram_frame_pos]);
      else if (sve4_pixfmt_eq(frame.format.pixfmt,
                              sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)))
        u32_frame_data[pos] = argb8(&frame_data[ram_frame_pos]);
    }
  }

  munit_assert_uint32(u32_frame_data[0], ==, 0xFF0000FF);
  munit_assert_uint32(u32_frame_data[1], ==, 0x00FF00FF);
  munit_assert_uint32(u32_frame_data[2], ==, 0x00FF00FF);
  munit_assert_uint32(u32_frame_data[3], ==, 0x000000FF);
  munit_assert_uint32(u32_frame_data[4], ==, 0x0000FFFF);
  munit_assert_uint32(u32_frame_data[5], ==, 0xFFFFFFFF);
  munit_assert_uint32(u32_frame_data[6], ==, 0xFFFF00FF);
  munit_assert_uint32(u32_frame_data[7], ==, 0x0000FFFF);
  munit_assert_uint32(u32_frame_data[8], ==, 0x0000FFFF);
  munit_assert_uint32(u32_frame_data[9], ==, 0x00FFFFFF);
  munit_assert_uint32(u32_frame_data[10], ==, 0xFF00FFFF);
  munit_assert_uint32(u32_frame_data[11], ==, 0x0000FFFF);
  munit_assert_uint32(u32_frame_data[12], ==, 0x000000FF);
  munit_assert_uint32(u32_frame_data[13], ==, 0x00FF00FF);
  munit_assert_uint32(u32_frame_data[14], ==, 0x00FF00FF);
  munit_assert_uint32(u32_frame_data[15], ==, 0xFF0000FF);

  sve4_decode_frame_free(&frame);
  sve4_decode_decoder_close(&decoder);
  return MUNIT_OK;
}

#ifdef SVE4_DECODE_HAVE_FFMPEG
static MunitResult test_multi_decode_webp(const MunitParameter params[],
                                          void* user_data) {
  (void)params;
  (void)user_data;

  const char* path = ASSETS_DIR "4x4.webp";

  sve4_decode_decoder_t decoders[3] = {0};
  sve4_decode_error_t err;
  sve4_buffer_ref_t demuxer = NULL;
  for (size_t i = 0; i < (sizeof(decoders) / sizeof(decoders[0])); ++i) {
    sve4_decode_decoder_config_t config = {
        .url = path,
        .backend = SVE4_DECODE_DECODER_BACKEND_FFMPEG,
        .demuxer = sve4_buffer_ref(demuxer),
    };
    err = sve4_decode_decoder_open(&decoders[i], &config);
    assert_success(err);
    if (!demuxer) {
      demuxer = sve4_decode_decoder_get_demuxer(&decoders[i]);
      munit_assert_ptr_not_null(demuxer);
    }
  }

  for (size_t i = 0; i < (sizeof(decoders) / sizeof(decoders[0])); ++i) {
    sve4_decode_frame_t frame = {0};
    err = sve4_decode_decoder_get_frame(&decoders[i], &frame, NULL);
    assert_success(err);

    munit_assert_int((int)frame.kind, ==, SVE4_DECODE_FRAME_KIND_RAM_FRAME);
    munit_assert_size(frame.width, ==, 4);
    munit_assert_size(frame.height, ==, 4);
    munit_assert_int((int)frame.format.kind, ==, SVE4_PIXFMT);
    munit_assert_true(
        sve4_pixfmt_eq(frame.format.pixfmt,
                       sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)) ||
        sve4_pixfmt_eq(frame.format.pixfmt,
                       sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
    sve4_decode_ram_frame_t* ram_frame = sve4_buffer_get_data(frame.buffer);
    const uint8_t* frame_data = ram_frame->data[0];
#pragma GCC diagnostic pop

    munit_assert_ptr_not_null(frame_data);
    uint32_t u32_frame_data[16];
    for (size_t row = 0; row < 4; ++row) {
      for (size_t col = 0; col < 4; ++col) {
        size_t pos = row * 4 + col;
        size_t ram_frame_pos = col * 4 + row * ram_frame->linesizes[0];
        if (sve4_pixfmt_eq(frame.format.pixfmt,
                           sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)))
          u32_frame_data[pos] = rgba8(&frame_data[ram_frame_pos]);
        else if (sve4_pixfmt_eq(frame.format.pixfmt,
                                sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)))
          u32_frame_data[pos] = argb8(&frame_data[ram_frame_pos]);
      }
    }

    munit_assert_uint32(u32_frame_data[0], ==, 0xFF0000FF);
    munit_assert_uint32(u32_frame_data[1], ==, 0x00FF00FF);
    munit_assert_uint32(u32_frame_data[2], ==, 0x00FF00FF);
    munit_assert_uint32(u32_frame_data[3], ==, 0x000000FF);
    munit_assert_uint32(u32_frame_data[4], ==, 0x0000FFFF);
    munit_assert_uint32(u32_frame_data[5], ==, 0xFFFFFFFF);
    munit_assert_uint32(u32_frame_data[6], ==, 0xFFFF00FF);
    munit_assert_uint32(u32_frame_data[7], ==, 0x0000FFFF);
    munit_assert_uint32(u32_frame_data[8], ==, 0x0000FFFF);
    munit_assert_uint32(u32_frame_data[9], ==, 0x00FFFFFF);
    munit_assert_uint32(u32_frame_data[10], ==, 0xFF00FFFF);
    munit_assert_uint32(u32_frame_data[11], ==, 0x0000FFFF);
    munit_assert_uint32(u32_frame_data[12], ==, 0x000000FF);
    munit_assert_uint32(u32_frame_data[13], ==, 0x00FF00FF);
    munit_assert_uint32(u32_frame_data[14], ==, 0x00FF00FF);
    munit_assert_uint32(u32_frame_data[15], ==, 0xFF0000FF);

    sve4_decode_frame_free(&frame);
    sve4_decode_decoder_close(&decoders[i]);
  }

  return MUNIT_OK;
}
#endif

static const MunitSuite test_suite = {
    "/generic",
    (MunitTest[]){
        {
            "/simple_webp",
            test_simple_webp,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"backend",
                 (char*[]){
                     "AUTO",
#ifdef SVE4_DECODE_HAVE_LIBWEBP
                     "LIBWEBP",
#endif
#ifdef SVE4_DECODE_HAVE_FFMPEG
                     "FFMPEG",
#endif
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
#ifdef SVE4_DECODE_HAVE_FFMPEG
        {
            "/multi_decode_webp",
            test_multi_decode_webp,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
#endif
        {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
         NULL} /* Mark the end of the array */
    },
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  sve4_log_test_setup();
  int ret = munit_suite_main(&test_suite, NULL, argc, argv);
  sve4_log_test_teardown();
  return ret;
}
