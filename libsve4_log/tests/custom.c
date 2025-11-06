#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "error.h"
#include "init.h"
#include "munit.h"

#define assert_success(err)                                                    \
  munit_assert_int((int)err, ==, SVE4_LOG_ERROR_SUCCESS)

static const char* custom_get_log_id_name(sve4_log_id_t log_id,
                                          sve4_buffer_ref_t user_data) {
  (void)user_data;
  switch (log_id) {
  case SVE4_LOG_ID_APPLICATION:
    return "custom-application";
  case SVE4_LOG_ID_DEFAULT_SVE4_DECODE:
    return "custom-sve4-decode";
  case SVE4_LOG_ID_DEFAULT_SVE4_LOG:
    return "custom-sve4-log";
  case SVE4_LOG_ID_DEFAULT_FFMPEG:
    return "custom-ffmpeg";
  case SVE4_LOG_ID_DEFAULT_VULKAN:
    return "custom-vulkan";
  default:;
  }

  return "idk";
}

static void custom_callback(sve4_log_record_t* record,
                            const sve4_log_config_t* conf) {
  (void)conf;
  (void)record;
}

static atomic_int log_init_refcount = 0;

static void* setup_log(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;
  if (atomic_fetch_add(&log_init_refcount, 1) > 0)
    return NULL;

  assert_success(sve4_log_init(NULL));

  sve4_log_config_t conf = {
      .level = SVE4_LOG_LEVEL_DEFAULT,
      .id_mapping =
          {
              .user_data = sve4_buffer_create(NULL, 42, NULL),
              .get_log_id_name = custom_get_log_id_name,
          },
      .path_shorten =
          {
              .max_length = 10,
              .root_prefix = SVE4_ROOT_DIR,
          },
  };

  {
    sve4_log_config_t conf_ref = sve4_log_config_ref(&conf);
    conf_ref.level = SVE4_LOG_LEVEL_WARNING;
    sve4_log_to_stderr(&conf_ref.callback, true);
    sve4_log_add_config(&conf_ref, NULL);
  }

  {
    FILE* file = tmpfile();
    munit_assert_not_null(file);
    sve4_log_config_t conf_ref = sve4_log_config_ref(&conf);
    conf_ref.level = SVE4_LOG_LEVEL_INFO;
    sve4_log_to_file(&conf_ref.callback, file, true, false);
    sve4_log_add_config(&conf_ref, NULL);
  }

  {
    sve4_log_config_t conf_ref = sve4_log_config_ref(&conf);
    sve4_log_to_munit(&conf_ref.callback);
    sve4_log_add_config(&conf_ref, NULL);
  }

  {
    sve4_log_config_t conf_ref = conf;
    conf_ref.callback = (sve4_log_callback_t){
        .user_data = sve4_buffer_create(NULL, 14, NULL),
        .callback = custom_callback,
    };
    sve4_log_add_config(&conf_ref, NULL);
  }
  return NULL;
}

static void teardown_log(void* user_data) {
  (void)user_data;
  if (atomic_fetch_sub(&log_init_refcount, 1) > 1)
    return;

  sve4_log_destroy();
}

static const MunitSuite test_suite = {
    "/custom",
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
