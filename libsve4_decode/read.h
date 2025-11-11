#pragma once

#include <stddef.h>

#include "sve4_decode_export.h"

#include "libsve4_decode/error.h"
#include "libsve4_utils/allocator.h"
#include "libsve4_utils/defines.h"

/**
 * @brief Reads data from a URL into a user-provided or newly allocated buffer.
 *
 * This function fetches data from the specified URL and writes it into the
 * buffer pointed to by @p *buffer. If @p *buffer is `NULL`, a new buffer of up
 * to
 * @p *bufsize bytes is allocated using @p alloc (if provided) or the default
 * allocator. Otherwise, the existing buffer is used, and at most @p *bufsize
 * bytes are written.
 *
 * On return, @p *bufsize is updated to the actual number of bytes read.
 *
 * @param alloc    Optional allocator used for memory allocation. If `NULL`, the
 * default allocation mechanism is used.
 * @param buffer   Input/output pointer to the data buffer. If `*buffer` is
 * `NULL`, memory is allocated by this function; otherwise, the existing buffer
 * is used.
 * @param bufsize  Input/output pointer to the buffer size. On input, it
 * specifies the maximum number of bytes to read. On output, it is set to the
 * number of bytes actually read.
 * @param url      The URL to read from. Must not be `NULL`.
 * @param binary   If `true`, data is read in binary mode; otherwise, in text
 * mode.
 *
 * @return An ::sve4_decode_error_t indicating success or the type of failure.
 *
 * @note The caller is responsible for freeing the allocated buffer (if any)
 *       using the same allocator passed in @p alloc.
 */
SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_read_url(sve4_allocator_t* _Nullable alloc,
                                         char* _Nullable* _Nonnull buffer,
                                         size_t* _Nonnull bufsize,
                                         const char* _Nonnull url, bool binary);
