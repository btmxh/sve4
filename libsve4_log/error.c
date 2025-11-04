#include "error.h"

const char* sve4_log_error_to_string(sve4_log_error_t err) {
  switch (err) {
  case SVE4_LOG_ERROR_SUCCESS:
    return "Success";
  case SVE4_LOG_ERROR_THREADS:
    return "Threading error";
  case SVE4_LOG_ERROR_MEMORY:
    return "Memory allocation error";
  }

  return "Unknown error";
}
