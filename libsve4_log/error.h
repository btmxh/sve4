#pragma once

#include "defines.h"

typedef enum {
  SVE4_LOG_ERROR_SUCCESS = 0,
  SVE4_LOG_ERROR_THREADS,
  SVE4_LOG_ERROR_MEMORY,
} sve4_log_error_t;

const char* _Nonnull sve4_log_error_to_string(sve4_log_error_t err);
