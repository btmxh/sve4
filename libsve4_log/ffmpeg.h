#pragma once

#include <stdio.h>

#include "defines.h"

void sve4_log_ffmpeg_callback(void* _Nullable avcl, int level,
                              const char* _Nonnull fmt, va_list args);
