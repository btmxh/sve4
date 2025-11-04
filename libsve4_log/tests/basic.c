#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "error.h"
#include "init.h"
#include "munit.h"

#define ASSETS_DIR "../../../../assets/"
enum { MS = (int64_t)1e6 };
#define ms *MS

#define assert_success(err)                                                    \
  do {                                                                         \
    munit_assert_int((int)err.source, ==, SVE4_DECODE_ERROR_SRC_DEFAULT);      \
    munit_assert_int((int)err.error_code, ==,                                  \
                     SVE4_DECODE_ERROR_DEFAULT_SUCCESS);                       \
  } while (0);

static MunitResult test_init(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;
  munit_assert_int((int)sve4_log_init(NULL), ==, (int)SVE4_LOG_ERROR_SUCCESS);
  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/webp",
    (MunitTest[]){
        {
            "/init",
            test_init,
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
