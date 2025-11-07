#include "glfw.h"

#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "init.h"
#include "munit.h"

#include "tests/counter.h"

#define assert_success(err)                                                    \
  munit_assert_int((int)err, ==, SVE4_LOG_ERROR_SUCCESS)

static atomic_int log_init_refcount = 0;

static void* setup_log(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;
  if (atomic_fetch_add(&log_init_refcount, 1) > 0)
    return NULL;

  assert_success(sve4_log_init(NULL));

  sve4_log_config_t conf = {
      .level = SVE4_LOG_LEVEL_DEFAULT,
      .id_mapping = sve4_log_id_mapping_default(),
      .path_shorten =
          {
              .max_length = 10,
              .root_prefix = SVE4_ROOT_DIR,
          },
  };
  sve4_log_to_stderr(&conf.callback, true);
  sve4_log_add_config(&conf, NULL);
  return NULL;
}

static void teardown_log(void* user_data) {
  if (atomic_fetch_sub(&log_init_refcount, 1) > 1)
    return;

  (void)user_data;
  sve4_log_destroy(); // intentionally omitted
}

static MunitResult test_basic_log(const MunitParameter params[],
                                  void* user_data) {
  (void)params;
  (void)user_data;

  sve4_log_t log;
  atomic_int num_logs = 0;
  sve4_log_add_config((sve4_log_config_t[]){counter_log_config(&num_logs)},
                      &log);
  sve4_log_glfw_callback(GLFW_NOT_INITIALIZED,
                         "The GLFW library is not initialized");
  munit_assert_int(atomic_load(&num_logs), ==, 1);
  sve4_log_glfw_callback(GLFW_NO_CURRENT_CONTEXT,
                         "There is no current context");
  munit_assert_int(atomic_load(&num_logs), ==, 2);
  assert_success(sve4_log_remove_log(log));

  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/glfw",
    (MunitTest[]){
        {
            "/basic_log",
            test_basic_log,
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
