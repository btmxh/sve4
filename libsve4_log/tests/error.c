#include "error.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "munit.h"

static MunitResult test_err_string(const MunitParameter params[],
                                   void* user_data) {
  (void)params;
  (void)user_data;

  munit_assert_string_equal(sve4_log_error_to_string(SVE4_LOG_ERROR_SUCCESS),
                            "Success");
  munit_assert_string_equal(sve4_log_error_to_string(SVE4_LOG_ERROR_MEMORY),
                            "Memory allocation error");
  munit_assert_string_equal(sve4_log_error_to_string(SVE4_LOG_ERROR_THREADS),
                            "Threading error");
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  munit_assert_string_equal(
      sve4_log_error_to_string((sve4_log_error_t)0xDEADBEEF), "Unknown error");
  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/log",
    (MunitTest[]){
        {
            "/err_string",
            test_err_string,
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
