#pragma once

#include <assert.h>

#include "libsve4_log/init.h"

#include <munit.h>

static void increase_log_counter(sve4_log_record_t* record,
                                 const sve4_log_config_t* config) {
  (void)record;
  assert(config->callback.user_data != NULL);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  atomic_int* counter =
      *(atomic_int**)sve4_buffer_get_data(config->callback.user_data);
#pragma GCC diagnostic pop
  atomic_fetch_add(counter, 1);
}

static inline sve4_log_config_t counter_log_config(atomic_int* counter) {
  sve4_log_config_t conf = {
      .level = SVE4_LOG_LEVEL_DEFAULT,
      .id_mapping = sve4_log_id_mapping_default(),
      .path_shorten =
          {
              .max_length = 10,
              .root_prefix = SVE4_ROOT_DIR,
          },
      .callback = {
          .callback = increase_log_counter,
          .user_data = sve4_buffer_create(NULL, sizeof(atomic_int*), NULL),
      }};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  munit_assert_not_null(conf.callback.user_data);
  *(atomic_int**)sve4_buffer_get_data(conf.callback.user_data) = counter;
#pragma GCC diagnostic pop
  return conf;
}
