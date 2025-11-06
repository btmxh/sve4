#include "tty.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "munit.h"

static MunitResult enable_ansi_stdin(const MunitParameter params[],
                                     void* user_data) {
  (void)params;
  (void)user_data;
  // may fail or not, depending on platform
  // since printing to stdin is the problem, not enabling ansi escape codes
  // there
  sve4_log_enable_ansi_escape_codes(stdin);
  return MUNIT_OK;
}

static MunitResult enable_ansi_stdout(const MunitParameter params[],
                                      void* user_data) {
  (void)params;
  (void)user_data;
  sve4_log_enable_ansi_escape_codes(stdout);
  return MUNIT_OK;
}

static MunitResult enable_ansi_stderr(const MunitParameter params[],
                                      void* user_data) {
  (void)params;
  (void)user_data;
  sve4_log_enable_ansi_escape_codes(stderr);
  return MUNIT_OK;
}

static MunitResult enable_ansi_tmpfile(const MunitParameter params[],
                                       void* user_data) {
  (void)params;
  (void)user_data;
  FILE* file = tmpfile();
  munit_assert_false(sve4_log_enable_ansi_escape_codes(file));
  fclose(file);
  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/basic",
    (MunitTest[]){
        {
            "/stdin",
            enable_ansi_stdin,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/stdout",
            enable_ansi_stdout,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/stderr",
            enable_ansi_stderr,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/tmpfile",
            enable_ansi_tmpfile,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
         NULL} /* Mark the end of the array */
    },
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
