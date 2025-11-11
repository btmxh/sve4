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

#include "http.h"

#ifdef unix
#include <fcntl.h>
#include <sys/stat.h>
#include <tinycthread.h>
#include <unistd.h>
#endif

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

#define PIPE_FIFO_URL "/tmp/unseekable.fifo"
static int test_read_pipe_alloc_write_thread(void* ptr) {
  (void)ptr;
  int fd = open(PIPE_FIFO_URL, O_WRONLY);
  assert(fd >= 0);
  const char msg[] = "Hello from thread FIFO!\n";
  ssize_t num_write = write(fd, msg, sizeof msg - 1);
  if (num_write != sizeof msg - 1)
    exit(1);
  close(fd);
  return 0;
}

static MunitResult test_read_pipe_alloc(const MunitParameter params[],
                                        void* user_data) {
  (void)params;
  (void)user_data;

  unlink(PIPE_FIFO_URL);
  munit_assert_int(mkfifo(PIPE_FIFO_URL, 0666), ==, 0);

  thrd_t write_thread;
  int thrd_err =
      thrd_create(&write_thread, test_read_pipe_alloc_write_thread, NULL);
  munit_assert_int(thrd_err, ==, thrd_success);

  char* buffer = NULL;
  size_t bufsize = SIZE_MAX;
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, PIPE_FIFO_URL, false);
  assert_success(err);

  munit_assert_memory_equal(bufsize, buffer, "Hello from thread FIFO!\n");
  sve4_free(NULL, buffer);

  int res = 0;
  thrd_join(write_thread, &res);
  munit_assert_int(res, ==, 0);

  unlink(PIPE_FIFO_URL);

  return MUNIT_OK;
}
#endif

static MunitResult test_read_http_basic(const MunitParameter params[],
                                        void* user_data) {
  (void)user_data;

  bool chunked = false;
  bool send_content_length = false;
  int delay_ms = 0;
  for (const MunitParameter* par = params; par->name; ++par) {
    if (strcmp(par->name, "chunked") == 0)
      chunked = strcmp(par->value, "true") == 0;
    else if (strcmp(par->name, "send_content_length") == 0)
      send_content_length = strcmp(par->value, "true") == 0;
    else if (strcmp(par->name, "delay_ms") == 0)
      delay_ms = atoi(par->value);
  }

  // FFmpeg's HTTP implementation requires either chunked encoding or
  // Content-Length to be present.
  if (!chunked && !send_content_length)
    return MUNIT_OK;

  int port = 0;
  test_http_server_t* server = test_http_server_start_auto(&port);
  munit_assert_int(port, !=, 0);

  const char data[] = "Hello, World! \r\n\0\x1\x2\n\x3\x4\0\0\0\1\2\3";
  size_t data_size = sizeof(data) - 1;

  test_http_response_t resp = {
      .data = data,
      .size = data_size,
      .type = TEST_HTTP_TEXT,
      .send_content_length = send_content_length,
      .chunked = chunked,
      .delay_ms = delay_ms,
  };

  test_http_server_add_static(server, "/test", &resp);

  char url[256];
  sprintf(url, "http://localhost:%d/test", port);

  char* buffer = NULL;
  size_t bufsize = SIZE_MAX;
  sve4_decode_error_t err =
      sve4_decode_read_url(NULL, &buffer, &bufsize, url, true);

  assert_success(err);
  munit_assert_ptr_not_null(buffer);
  munit_assert_size(bufsize, ==, data_size);
  munit_assert_memory_equal(data_size, buffer, data);

  sve4_free(NULL, buffer);

  test_http_server_stop(server);
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
                     ASSETS_DIR "alice.unix.txt",
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
                     ASSETS_DIR "alice.unix.txt",
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
                     ASSETS_DIR "alice.unix.txt",
#ifdef _WIN32
                     ASSETS_DIR "alice.dos.txt",
#endif
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/text/read_file_overflow",
            test_text_read_file_overflow,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.unix.txt",
#ifdef _WIN32
                     ASSETS_DIR "alice.dos.txt",
#endif
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/text/read_file_overflow_alloc",
            test_text_read_file_overflow_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.unix.txt",
#ifdef _WIN32
                     ASSETS_DIR "alice.dos.txt",
#endif
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/binary/read_file_alloc",
            test_binary_read_file_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.unix.txt",
                     ASSETS_DIR "alice.dos.txt",
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/binary/read_file_truncated",
            test_binary_read_file_truncated,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.unix.txt",
                     ASSETS_DIR "alice.dos.txt",
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/binary/read_file_overflow",
            test_binary_read_file_overflow,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"url",
                 (char*[]){
                     ASSETS_DIR "alice.unix.txt",
                     ASSETS_DIR "alice.dos.txt",
                     NULL,
                 }},
                {NULL, NULL},
            },
        },
        {
            "/binary/real_alice",
            test_read_real_alice,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
#ifdef unix
        {
            "/text/fifo",
            test_read_pipe_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/binary/dev_null",
            test_read_file_dev_null,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/binary/dev_null_alloc",
            test_read_file_dev_null_alloc,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/binary/dev_zero",
            test_read_file_dev_zero,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
#endif
        {
            "/http/basic",
            test_read_http_basic,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            (MunitParameterEnum[]){
                {"chunked", (char*[]){"true", "false", NULL}},
                {"send_content_length", (char*[]){"true", "false", NULL}},
                {"delay_ms", (char*[]){"0", "10", NULL}},
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
