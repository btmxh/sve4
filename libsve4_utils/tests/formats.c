#include <stdbool.h>

#include <libsve4_utils/formats.h>
#include <munit.h>

#ifdef SVE4_UTILS_HAVE_FFMPEG
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#endif

static MunitResult test_to_string(const MunitParameter params[],
                                  void* user_data) {
  (void)params;
  (void)user_data;

  munit_assert_string_equal(
      sve4_pixfmt_to_string(sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)),
      "rgba");
  munit_assert_string_equal(
      sve4_pixfmt_to_string(sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)),
      "argb");
  munit_assert_string_equal(sve4_samplefmt_to_string(sve4_samplefmt_default(
                                SVE4_SAMPLEFMT_DEFAULT_S16)),
                            "s16");

#ifdef SVE4_UTILS_HAVE_FFMPEG
  munit_assert_string_equal(
      sve4_pixfmt_to_string(sve4_pixfmt_ffmpeg(AV_PIX_FMT_RGBA)), "rgba");
  munit_assert_string_equal(
      sve4_pixfmt_to_string(sve4_pixfmt_ffmpeg(AV_PIX_FMT_ARGB)), "argb");
  munit_assert_string_equal(
      sve4_pixfmt_to_string(sve4_pixfmt_ffmpeg(AV_PIX_FMT_ABGR)), "abgr");
  munit_assert_string_equal(sve4_samplefmt_to_string(sve4_samplefmt_ffmpeg(
                                SVE4_SAMPLEFMT_DEFAULT_S16)),
                            "s16");
#endif
  return MUNIT_OK;
}

#ifdef SVE4_UTILS_HAVE_FFMPEG
static MunitResult test_ffmpeg_pixfmt_to_string(const MunitParameter params[],
                                                void* user_data) {
  (void)params;
  (void)user_data;

  for (enum AVPixelFormat fmt = 0; fmt < AV_PIX_FMT_NB; fmt++) {
    const char* name = av_get_pix_fmt_name(fmt);
    if (name) {
      munit_assert_string_equal(sve4_pixfmt_to_string(sve4_pixfmt_ffmpeg(fmt)),
                                name);
    }
  }

  return MUNIT_OK;
}
#endif

static MunitResult test_pixfmt_num_planes(const MunitParameter params[],
                                          void* user_data) {
  (void)params;
  (void)user_data;

  munit_assert_size(
      sve4_pixfmt_num_planes(sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)),
      ==, 1);
  munit_assert_size(
      sve4_pixfmt_num_planes(sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)),
      ==, 1);
#ifdef SVE4_UTILS_HAVE_FFMPEG
  munit_assert_size(
      sve4_pixfmt_num_planes(sve4_pixfmt_ffmpeg(AV_PIX_FMT_YUV420P)), ==, 3);
#endif
  return MUNIT_OK;
}

static MunitResult test_pixfmt_linesize(const MunitParameter params[],
                                        void* user_data) {
  (void)params;
  (void)user_data;

  size_t linesize = sve4_pixfmt_linesize(
      sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8), 0, 1920, 16);
  munit_assert_size(linesize, ==, 1920 * 4);

  return MUNIT_OK;
}

static MunitResult test_pixfmt_canonicalize(const MunitParameter params[],
                                            void* user_data) {
  (void)params;
  (void)user_data;

#ifdef SVE4_UTILS_HAVE_FFMPEG
  munit_assert_true(sve4_pixfmt_eq(
      sve4_pixfmt_canonicalize(sve4_pixfmt_ffmpeg(AV_PIX_FMT_RGBA)),
      sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_RGBA8)));
  munit_assert_true(sve4_pixfmt_eq(
      sve4_pixfmt_canonicalize(sve4_pixfmt_ffmpeg(AV_PIX_FMT_ARGB)),
      sve4_pixfmt_default(SVE4_PIXFMT_DEFAULT_ARGB8)));
#endif

  return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
    {
        "/to_string",
        test_to_string,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/ffmpeg_to_string",
        test_ffmpeg_pixfmt_to_string,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/pixfmt_num_planes",
        test_pixfmt_num_planes,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/pixfmt_linesize",
        test_pixfmt_linesize,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {
        "/pixfmt_canonicalize",
        test_pixfmt_canonicalize,
        NULL,
        NULL,
        MUNIT_TEST_OPTION_NONE,
        NULL,
    },
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL} /* Mark the end of the array */
};

static const MunitSuite test_suite = {
    "/formats", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
