#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "sve4_log_export.h"

SVE4_LOG_EXPORT
bool sve4_log_enable_ansi_escape_codes(FILE* file);
