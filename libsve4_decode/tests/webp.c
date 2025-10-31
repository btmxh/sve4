#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "libwebp.h"
#include "munit.h"
#include "ram_frame.h"

static void read_binary_file(const char* path, void** data, size_t* size) {
  assert(data && size);
  FILE* f = fopen(path, "rb");
  munit_assert_ptr_not_null(f);
  fseek(f, 0, SEEK_END);
  *size = (size_t)ftell(f);
  *data = sve4_malloc(NULL, *size);
  fseek(f, 0, SEEK_SET);
  size_t num_read = (size_t)fread(*data, 1, *size, f);
  munit_assert_size(*size, ==, num_read);
  fclose(f);
}

#define assert_success(err)                                                    \
  do {                                                                         \
    munit_assert_int(err.source, ==, SVE4_DECODE_ERROR_SRC_DEFAULT);           \
    munit_assert_int(err.error_code, ==, SVE4_DECODE_ERROR_DEFAULT_SUCCESS);   \
  } while (0);

static MunitResult test_simple_webp_demux(const MunitParameter params[],
                                          void* user_data) {
  (void)params;
  (void)user_data;

  const char* path = "../../../assets/4x4.webp";
  void* data = NULL;
  size_t size = 0;
  read_binary_file(path, &data, &size);
  munit_assert_size(size, >, 0);

  sve4_decode_libwebp_demux_t demux;
  sve4_decode_error_t err;
  err = sve4_decode_libwebp_open_demux(&demux, data, size);
  assert_success(err);

  munit_assert_size(sve4_decode_libwebp_demux_get_width(&demux), ==, 4);
  munit_assert_size(sve4_decode_libwebp_demux_get_height(&demux), ==, 4);
  munit_assert_size(sve4_decode_libwebp_demux_get_num_frames(&demux), ==, 1);

  sve4_decode_libwebp_close_demux(&demux);
  sve4_free(NULL, data);
  return MUNIT_OK;
}

static MunitResult test_simple_webp_anim(const MunitParameter params[],
                                         void* user_data) {
  (void)params;
  (void)user_data;

  const char* path = "../../../assets/4x4.webp";
  void* data = NULL;
  size_t size = 0;
  read_binary_file(path, &data, &size);
  munit_assert_size(size, >, 0);

  sve4_decode_libwebp_anim_t anim;
  sve4_decode_error_t err;
  err = sve4_decode_libwebp_open_anim(&anim, data, size);
  assert_success(err);

  munit_assert_size(sve4_decode_libwebp_anim_get_width(&anim), ==, 4);
  munit_assert_size(sve4_decode_libwebp_anim_get_height(&anim), ==, 4);
  munit_assert_size(sve4_decode_libwebp_anim_get_num_frames(&anim), ==, 1);

  munit_assert_true(sve4_decode_libwebp_anim_has_more(&anim));
  sve4_decode_frame_t frame;
  sve4_decode_libwebp_anim_alloc(&anim, NULL, &frame);
  err = sve4_decode_libwebp_anim_decode(&anim, &frame);
  assert_success(err);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullable-to-nonnull-conversion"
  sve4_decode_ram_frame_t* ram_frame = sve4_buffer_get_data(frame.buffer);
  const uint8_t* frame_data = ram_frame->data[0];
#pragma clang diagnostic pop

  munit_assert_ptr_not_null(frame_data);
  uint32_t u32_frame_data[16];
  for (size_t i = 0; i < 16; ++i) {
    u32_frame_data[i] = frame_data[i * 4 + 0] << 24 |
                        frame_data[i * 4 + 1] << 16 |
                        frame_data[i * 4 + 2] << 8 | frame_data[i * 4 + 3];
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

  munit_assert_false(sve4_decode_libwebp_anim_has_more(&anim));

  sve4_decode_libwebp_close_anim(&anim);
  sve4_free(NULL, data);
  return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {
        "/simple_webp_demux",
        test_simple_webp_demux,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/simple_webp_anim",
        test_simple_webp_anim,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL} /* Mark the end of the array */
};

static const MunitSuite test_suite = {
    "/allocator", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
