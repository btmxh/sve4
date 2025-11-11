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

  const char* url = NULL;
  for (const MunitParameter* par = params; par->name; ++par) {
    if (strcmp(par->name, "url") == 0)
      url = par->value;
  }
  munit_assert_not_null(url);

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

static MunitResult
test_text_read_file_truncated_alloc(const MunitParameter params[],
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
#define BUFFER_SIZE 16
  size_t bufsize = BUFFER_SIZE;
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, false);
  assert_success(err);

  munit_assert_ptr_not_null(buffer);
  munit_assert_size(bufsize, ==, BUFFER_SIZE);

  char text[BUFFER_SIZE + 1];
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(text, buffer, BUFFER_SIZE);
  text[BUFFER_SIZE] = '\0';
#undef BUFFER_SIZE

  munit_assert_string_equal(text, "guys they make a");

  sve4_free(NULL, buffer);

  return MUNIT_OK;
}

static MunitResult test_text_read_file_overflow(const MunitParameter params[],
                                                void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = NULL;
  for (const MunitParameter* par = params; par->name; ++par) {
    if (strcmp(par->name, "url") == 0)
      url = par->value;
  }
  munit_assert_not_null(url);

  char buffer[2048];
  size_t bufsize = sizeof(buffer);
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &(char*){buffer}, &bufsize, url, false);
  assert_success(err);

  munit_assert_size(bufsize, ==, 117);
  buffer[117] = '\0';
  munit_assert_string_equal(
      buffer, "guys they make a new song about a robot after the end of the "
              "world to diss ksdgk\nfirst album is so back wtf lmfaoooo\n");

  return MUNIT_OK;
}

static MunitResult
test_text_read_file_overflow_alloc(const MunitParameter params[],
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
#define BUFFER_SIZE 2048
  size_t bufsize = BUFFER_SIZE;
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, false);
  assert_success(err);

  munit_assert_size(bufsize, ==, 117);
  buffer[117] = '\0';
  munit_assert_string_equal(
      buffer, "guys they make a new song about a robot after the end of the "
              "world to diss ksdgk\nfirst album is so back wtf lmfaoooo\n");

  sve4_free(NULL, buffer);

  return MUNIT_OK;
}

static bool is_dos_txt_file(const char* url) {
#define SUFFIX_DOS_TXT ".dos.txt"
#define SUFFIX_DOS_TXT_LEN (sizeof(SUFFIX_DOS_TXT) - 1)
  size_t url_len = strlen(url);
  if (url_len < SUFFIX_DOS_TXT_LEN)
    return false;
  return strcmp(&url[url_len - SUFFIX_DOS_TXT_LEN], SUFFIX_DOS_TXT) == 0;
}

static MunitResult test_binary_read_file_alloc(const MunitParameter params[],
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
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, true);
  assert_success(err);

  munit_assert_ptr_not_null(buffer);

  size_t expected_size = is_dos_txt_file(url) ? 119 : 117;
  munit_assert_size(bufsize, ==, expected_size);

  char text[120];
  memcpy(text, buffer, bufsize);
  text[bufsize] = '\0';

  munit_assert_string_equal(
      text,
      is_dos_txt_file(url)
          ? "guys they make a new song about a robot after the end of the "
            "world to diss ksdgk\r\nfirst album is so back wtf lmfaoooo\r\n"
          : "guys they make a new song about a robot after the end of the "
            "world to diss ksdgk\nfirst album is so back wtf lmfaoooo\n");

  sve4_free(NULL, buffer);
  return MUNIT_OK;
}

static MunitResult
test_binary_read_file_truncated(const MunitParameter params[],
                                void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = NULL;
  for (const MunitParameter* par = params; par->name; ++par) {
    if (strcmp(par->name, "url") == 0)
      url = par->value;
  }
  munit_assert_not_null(url);

  char buffer[16];
  size_t bufsize = sizeof(buffer);
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &(char*){buffer}, &bufsize, url, true);
  assert_success(err);

  munit_assert_size(bufsize, ==, sizeof(buffer));

  char text[sizeof(buffer) + 1];
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  memcpy(text, buffer, sizeof(buffer));
  text[sizeof(buffer)] = '\0';

  munit_assert_string_equal(text, "guys they make a");

  return MUNIT_OK;
}

static MunitResult test_binary_read_file_overflow(const MunitParameter params[],
                                                  void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = NULL;
  for (const MunitParameter* par = params; par->name; ++par) {
    if (strcmp(par->name, "url") == 0)
      url = par->value;
  }
  munit_assert_not_null(url);

  char buffer[2048];
  size_t bufsize = sizeof(buffer);
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &(char*){buffer}, &bufsize, url, true);
  assert_success(err);

  size_t expected_size = is_dos_txt_file(url) ? 119 : 117;
  munit_assert_size(bufsize, ==, expected_size);
  buffer[bufsize] = '\0';

  munit_assert_string_equal(
      buffer,
      is_dos_txt_file(url)
          ? "guys they make a new song about a robot after the end of the "
            "world to diss ksdgk\r\nfirst album is so back wtf lmfaoooo\r\n"
          : "guys they make a new song about a robot after the end of the "
            "world to diss ksdgk\nfirst album is so back wtf lmfaoooo\n");

  return MUNIT_OK;
}

static MunitResult test_read_real_alice(const MunitParameter params[],
                                        void* user_data) {
  (void)params;
  (void)user_data;

  // FFmpeg doesn't support HTTPS in the CI vcpkg configuration
  const char* url = "http://gaia.cs.umass.edu/wireshark-labs/alice.txt";

  char* buffer = NULL;
  size_t bufsize = SIZE_MAX;

  // FFmpeg does not support reading text using the avio API, so we can't
  // really set binary to false here.
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, true);

  assert_success(err);

  munit_assert_ptr_not_null(buffer);
  munit_assert_size(bufsize, ==, 152138);

  // HTTP line separators are \r\n, and since we read in binary mode, we
  // expect to see them as-is.
  const char expected_header[] =
      "                ALICE'S ADVENTURES IN WONDERLAND\r\n"
      "\r\n"
      "                          Lewis Carroll\r\n";
  char header[sizeof(expected_header)];
  memcpy(header, buffer, sizeof(expected_header) - 1);
  header[sizeof(expected_header) - 1] = '\0';

  munit_assert_string_equal(header, expected_header);

  sve4_free(NULL, buffer);
  return MUNIT_OK;
}

#ifdef unix
static MunitResult test_read_file_dev_null_alloc(const MunitParameter params[],
                                                 void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = "/dev/null";

  char* buffer = NULL;
  size_t bufsize = SIZE_MAX;

  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, true);

  assert_success(err);

  munit_assert_size(bufsize, ==, 0);

  sve4_free(NULL, buffer);
  return MUNIT_OK;
}

static MunitResult test_read_file_dev_null(const MunitParameter params[],
                                           void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = "/dev/null";

  char buffer[1024];
  size_t bufsize = sizeof(buffer);

  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &(char*){buffer}, &bufsize, url, true);

  assert_success(err);

  munit_assert_size(bufsize, ==, 0);

  return MUNIT_OK;
}

static MunitResult test_read_file_dev_zero(const MunitParameter params[],
                                           void* user_data) {
  (void)params;
  (void)user_data;

  const char* url = "/dev/zero";

  char buffer[1024];
  size_t bufsize = sizeof(buffer);

  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &(char*){buffer}, &bufsize, url, true);

  assert_success(err);

  munit_assert_size(bufsize, ==, sizeof(buffer));

  for (size_t i = 0; i < bufsize; ++i) {
    munit_assert_uint8((uint8_t)buffer[i], ==, 0);
  }

  return MUNIT_OK;
}
#endif

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
        {
            "/text/read_file_truncated_alloc",
            test_text_read_file_truncated_alloc,
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
            "text/read_file_overflow",
            test_text_read_file_overflow,
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
            "text/read_file_overflow_alloc",
            test_text_read_file_overflow_alloc,
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
            "binary/read_file_alloc",
            test_binary_read_file_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.txt",
                     ASSETS_DIR "alice.dos.txt",
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "binary/read_file_truncated",
            test_binary_read_file_truncated,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.txt",
                     ASSETS_DIR "alice.dos.txt",
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "binary/read_file_overflow",
            test_binary_read_file_overflow,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.txt",
                     ASSETS_DIR "alice.dos.txt",
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "binary/real_alice",
            test_read_real_alice,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
#ifdef unix
        {
            "binary/dev_null",
            test_read_file_dev_null,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "binary/dev_null_alloc",
            test_read_file_dev_null_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "binary/dev_zero",
            test_read_file_dev_zero,
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
