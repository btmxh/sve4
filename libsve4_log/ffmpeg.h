#pragma once

#include <stdio.h>

#include "sve4_log_export.h"

#include <libsve4_utils/defines.h>

SVE4_LOG_EXPORT
void sve4_log_ffmpeg_callback(void* _Nullable avcl, int level,
                              const char* _Nonnull fmt, va_list args);
