#pragma once

#include <stddef.h>

#include "sve4_decode_export.h"

#include "libsve4_decode/error.h"
#include "libsve4_utils/allocator.h"
#include "libsve4_utils/defines.h"

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_read_url(sve4_allocator_t* _Nullable alloc,
                                         char* _Nullable* _Nonnull buffer,
                                         size_t* _Nonnull bufsize,
                                         const char* _Nonnull url, bool binary,
                                         size_t max_read);
