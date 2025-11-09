#pragma once

#include "defines.h"

typedef struct {
  const char* _Nonnull file;
  int line;
} sve4_error_trace_t;

#define SVE4_ERROR_TRACE_HERE ((sve4_error_trace_t){__FILE__, __LINE__})
