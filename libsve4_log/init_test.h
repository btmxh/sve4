#pragma once

#include "sve4_log_export.h"

#ifdef SVE4_LOG_HAVE_MUNIT
#include <munit.h>
#endif

SVE4_LOG_EXPORT
void sve4_log_test_setup(void);
SVE4_LOG_EXPORT
void sve4_log_test_teardown(void);

// munit callbacks
#ifdef SVE4_LOG_HAVE_MUNIT
SVE4_LOG_EXPORT
void* sve4_log_test_setup_func(const MunitParameter params[], void* user_data);
SVE4_LOG_EXPORT
void sve4_log_test_teardown_func(void* user_data);
#endif
