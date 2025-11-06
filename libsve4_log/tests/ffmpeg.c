#include "ffmpeg.h"

#include <assert.h>
#include <libavutil/log.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "init.h"
#include "munit.h"

#define assert_success(err)                                                    \
  munit_assert_int((int)err, ==, SVE4_LOG_ERROR_SUCCESS)

static atomic_int log_init_refcount = 0;
static atomic_int num_logs = 0;

static void log_callback_inc_counter(sve4_log_record_t* record,
                                     const sve4_log_config_t* config) {
  (void)record;
  (void)config;
  atomic_fetch_add(&num_logs, 1);
}

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

  {
    sve4_log_config_t conf_ref = sve4_log_config_ref(&conf);
    sve4_log_to_stderr(&conf_ref.callback, true);
    sve4_log_add_config(&conf_ref, NULL);
  }

  {
    sve4_log_config_t conf_ref = conf;
    conf_ref.callback = (sve4_log_callback_t){
        .callback = log_callback_inc_counter,
        .user_data = NULL,
    };
    sve4_log_add_config(&conf_ref, NULL);
  }
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

  atomic_store(&num_logs, 0);
  av_log(NULL, AV_LOG_ERROR, "Hello, %s\n", "World!!");
  av_log(NULL, AV_LOG_ERROR, "Hello, %s\n", "World!!");
  av_log(NULL, AV_LOG_ERROR, "Hello, %s\n", "World!!");
  av_log(NULL, AV_LOG_ERROR, "Hello, %s\n", "World!!");
  munit_assert_int(atomic_load(&num_logs), ==, 4);
  return MUNIT_OK;
}

static MunitResult test_ignored_log(const MunitParameter params[],
                                    void* user_data) {
  (void)params;
  (void)user_data;

  atomic_store(&num_logs, 0);
  av_log(NULL, AV_LOG_TRACE, "Hello, %s\n", "World!!");
  av_log(NULL, AV_LOG_TRACE, "Hello, %s\n", "World!!");
  av_log(NULL, AV_LOG_TRACE, "Hello, %s\n", "World!!");
  av_log(NULL, AV_LOG_TRACE, "Hello, %s\n", "World!!");
  munit_assert_int(atomic_load(&num_logs), ==, 4);
  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/basic",
    (MunitTest[]){
        {
            "/basic_log",
            test_basic_log,
            setup_log,
            teardown_log,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {
            "/ignored_log",
            test_ignored_log,
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
  av_log_set_level(AV_LOG_ERROR); // does not matter
  av_log_set_callback(sve4_log_ffmpeg_callback);
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
