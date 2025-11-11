#include "libsve4_decode/read.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libsve4_log/init_test.h"

#include <libsve4_decode/error.h>
#include <libsve4_decode/libwebp.h>
#include <libsve4_decode/ram_frame.h>
#include <libsve4_utils/allocator.h>
#include <munit.h>

#define ASSETS_DIR "../../../../assets/"

#define assert_success(err)                                                    \
  do {                                                                         \
    munit_assert_int((int)err.source, ==, SVE4_DECODE_ERROR_SRC_DEFAULT);      \
    munit_assert_int((int)err.error_code, ==,                                  \
                     SVE4_DECODE_ERROR_DEFAULT_SUCCESS);                       \
  } while (0);

static MunitResult test_text_read_file_alloc(const MunitParameter params[],
                                             void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = NULL;
  for (const MunitParameter* par = params; par->name; ++par) {
    if (strcmp(par->name, "url") == 0)
      url = par->value;
  }
  munit_assert_not_null(url);

  char* buffer = NULL;
  size_t bufsize = SIZE_MAX;
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, false);
  assert_success(err);

#define FILE_SIZE 117
  munit_assert_ptr_not_null(buffer);
  munit_assert_size(bufsize, ==, FILE_SIZE);

  char text[FILE_SIZE + 1];
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(text, buffer, FILE_SIZE);
  text[FILE_SIZE] = '\0';
#undef FILE_SIZE

  munit_assert_string_equal(
      text, "guys they make a new song about a robot after the end of the "
            "world to diss ksdgk\nfirst album is so back wtf lmfaoooo\n");

  sve4_free(NULL, buffer);
  return MUNIT_OK;
}

static MunitResult test_text_read_file_truncated(const MunitParameter params[],
                                                 void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = ASSETS_DIR "alice.txt";

  char buffer[16];
  size_t bufsize = sizeof(buffer);
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &(char*){buffer}, &bufsize, url, false);
  assert_success(err);

  munit_assert_size(bufsize, ==, sizeof(buffer));

  char text[sizeof(buffer) + 1];
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(text, buffer, sizeof(buffer));
  text[sizeof(buffer)] = '\0';

  munit_assert_string_equal(text, "guys they make a");

  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/read",
    (MunitTest[]){
        {
            "/text/read_file",
            test_text_read_file_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.txt",
#ifdef _WIN32
                     ASSETS_DIR "alice.dos.txt",
#endif
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/text/read_file_truncated",
            test_text_read_file_truncated,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.txt",
#ifdef _WIN32
                     ASSETS_DIR "alice.dos.txt",
#endif
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
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
