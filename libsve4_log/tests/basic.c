#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "api.h"
#include "error.h"
#include "init.h"
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

static MunitResult test_app_log(const MunitParameter params[],
                                void* user_data) {
  (void)params;
  (void)user_data;
  int a = 621;
  double b = 14;
  long long c = 42;
  const char* msg = "%s %d";
  sve4_log_debug("Hi, a = %d, b = %lf", a, b);
  sve4_log_info("c = %lld, msg = \"%s\"", c, msg);
  sve4_log_warn("Hello, World!");
  munit_log(MUNIT_LOG_INFO, "Hi");
  // munit panic on error
  // sve4_log_error("Error!");

  return MUNIT_OK;
}

static MunitResult test_external_log(const MunitParameter params[],
                                     void* user_data) {
  (void)params;
  (void)user_data;
  sve4_flog(SVE4_LOG_ID_DEFAULT_FFMPEG, SVE4_LOG_LEVEL_INFO,
            "This is an external log: %d", 456);
  sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_INFO,
            "This is an sve4-log log: %d", 456);
  sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_DECODE, SVE4_LOG_LEVEL_INFO,
            "This is an sve4-decode log: %d", 456);
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  sve4_flog(42, SVE4_LOG_LEVEL_INFO, "This is an external log from 42: %d",
            727);
  sve4_glog(SVE4_LOG_ID_DEFAULT_VULKAN, "\\usr\\include\\vulkan\\vulkan.hpp",
            621, true, SVE4_LOG_LEVEL_WARNING,
            "Vulkan validation layer warning: %s", "warning message");
  return MUNIT_OK;
}

static MunitResult test_freestanding_log(const MunitParameter params[],
                                         void* user_data) {
  (void)params;
  (void)user_data;
  sve4_flog(SVE4_LOG_ID_APPLICATION, SVE4_LOG_LEVEL_INFO,
            "This is a freestanding log: %d", 123);
  return MUNIT_OK;
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
