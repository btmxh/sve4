#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "allocator.h"
#include "api.h"
#include "buffer.h"
#include "defines.h"
#include "error.h"
#include "sve4_log_export.h"

typedef struct {
  sve4_log_level_t level;
  sve4_log_id_t id;
  const char* _Nonnull msg;
  va_list args;
  const struct tm* _Nonnull timestamp;
  int32_t fractional_timestamp;
  const char* _Nonnull file;
  size_t line;
  bool endl;
} sve4_log_record_t;

typedef struct sve4_log_config_t sve4_log_config_t;
typedef void (*sve4_log_callback_fn_t)(sve4_log_record_t* _Nonnull record,
                                       const sve4_log_config_t* _Nonnull conf);

typedef struct {
  sve4_log_callback_fn_t _Nonnull callback;
  sve4_buffer_ref_t _Nullable user_data;
} sve4_log_callback_t;
SVE4_LOG_EXPORT
sve4_log_callback_t sve4_log_callback_ref(sve4_log_callback_t src);
SVE4_LOG_EXPORT
void sve4_log_callback_free(sve4_log_callback_t* _Nullable callback);

enum { SVE4_LOG_SHORTEN_PATH_MAX_LENGTH = 64 };

typedef struct {
  size_t max_length; // 0 -> no trimming, must be <=
                     // SVE4_LOG_SHORTEN_PATH_MAX_LENGTH
  const char* _Nullable root_prefix;
  sve4_buffer_ref_t _Nullable user_data; // potentially storing root_prefix
} sve4_log_shorten_path_config_t;
SVE4_LOG_EXPORT
sve4_log_shorten_path_config_t
sve4_log_shorten_path_config_ref(sve4_log_shorten_path_config_t src);
SVE4_LOG_EXPORT
void sve4_shorten_path_config_free(
    sve4_log_shorten_path_config_t* _Nullable config);

typedef struct {
  const char* _Nullable (*_Nonnull get_log_id_name)(
      sve4_log_id_t log_id, sve4_buffer_ref_t _Nullable user_data);
  sve4_buffer_ref_t _Nullable user_data;
} sve4_log_id_mapping_t;
SVE4_LOG_EXPORT
sve4_log_id_mapping_t sve4_log_id_mapping_ref(sve4_log_id_mapping_t src);
SVE4_LOG_EXPORT
sve4_log_id_mapping_t sve4_log_id_mapping_default(void);
SVE4_LOG_EXPORT
void sve4_log_id_mapping_free(sve4_log_id_mapping_t* _Nullable mapping);

struct sve4_log_config_t {
  sve4_log_level_t level;
  sve4_log_callback_t callback;
  sve4_log_shorten_path_config_t path_shorten;
  sve4_log_id_mapping_t id_mapping;
};

SVE4_LOG_EXPORT
sve4_log_config_t sve4_log_config_ref(const sve4_log_config_t* src);
SVE4_LOG_EXPORT
sve4_log_error_t sve4_log_init(sve4_allocator_t* _Nullable allocator);
SVE4_LOG_EXPORT
void sve4_log_destroy(void);

typedef struct sve4_log_inner_t* sve4_log_t;

// NOTE: this move the config ownership to the log system, so do not free it
SVE4_LOG_EXPORT
sve4_log_error_t sve4_log_add_config(sve4_log_config_t* _Nonnull config,
                                     sve4_log_t _Nullable* _Nullable log);
SVE4_LOG_EXPORT
sve4_log_error_t sve4_log_remove_log(sve4_log_t _Nonnull log);

SVE4_LOG_EXPORT
sve4_log_error_t sve4_log_to_file(sve4_log_callback_t* _Nonnull callback,
                                  FILE* _Nonnull file, bool close, bool ansi);
SVE4_LOG_EXPORT
sve4_log_error_t sve4_log_to_stderr(sve4_log_callback_t* _Nonnull callback,
                                    bool force_ansi);
#ifdef SVE4_HAS_MUNIT
SVE4_LOG_EXPORT
sve4_log_error_t sve4_log_to_munit(sve4_log_callback_t* _Nonnull callback);
#endif
