#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "libsve4_log/init.h"

#include "common.h"
#include "munit.h"

#define assert_success(err)                                                    \
  munit_assert_int((int)err, ==, SVE4_LOG_ERROR_SUCCESS)

static sve4_log_config_t default_config(void) {
  return (sve4_log_config_t){
      .level = SVE4_LOG_LEVEL_DEFAULT,
      .id_mapping = sve4_log_id_mapping_default(),
      .path_shorten =
          {
              .max_length = 10,
              .root_prefix = SVE4_ROOT_DIR,
          },
  };
}

static atomic_int log_init_refcount = 0;

static void* setup_log(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;
  if (atomic_fetch_add(&log_init_refcount, 1) > 0)
    return NULL;

  assert_success(sve4_log_init(NULL));

  sve4_log_config_t conf = default_config();
  // NOTE: current conf does not allocate anything
  sve4_log_to_stderr(&conf.callback, true);
  sve4_log_add_config(&conf, NULL);

  conf = default_config();
  sve4_log_to_munit(&conf.callback);

  sve4_log_add_config(&conf, NULL);
  return NULL;
}

static void teardown_log(void* user_data) {
  if (atomic_fetch_sub(&log_init_refcount, 1) > 1)
    return;

  (void)user_data;
  sve4_log_destroy(); // intentionally omitted
}

static const MunitSuite test_suite = {
    "/basic",
    (MunitTest[]){
        {
            "/init",
            test_app_log,
            setup_log,
            teardown_log,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/freestanding",
            test_freestanding_log,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/external",
            test_external_log,
            setup_log,
            teardown_log,
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
