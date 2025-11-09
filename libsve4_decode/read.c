#include "read.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sve4_decode_export.h"

#include "libsve4_decode/error.h"
#include "libsve4_log/api.h"
#include "libsve4_utils/allocator.h"
#include "libsve4_utils/arena.h"
#include "libsve4_utils/defines.h"

#include <libavutil/error.h>

#ifdef SVE4_DECODE_HAVE_FFMPEG
#include <libavformat/avio.h>
#endif

static sve4_decode_error_t
sve4_decode_read_file_stdio(sve4_allocator_t* _Nullable alloc,
                            char* _Nullable* _Nonnull buffer,
                            size_t* _Nonnull bufsize, const char* _Nonnull url,
                            bool binary, size_t max_read) {
#define FILE_PREFIX "file://"
#define FILE_PREFIX_LEN (sizeof(FILE_PREFIX) - 1)
  if (strncmp(url, FILE_PREFIX, FILE_PREFIX_LEN) == 0)
    url += FILE_PREFIX_LEN;

  FILE* file = fopen(url, binary ? "rb" : "r");
  if (!file)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);

  size_t to_read = max_read;

  if (max_read == SIZE_MAX) {
    // determine file size
    if (fseek(file, 0, SEEK_END) != 0) {
      fclose(file);
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
    }

    long fsize = ftell(file);
    if (fsize < 0) {
      fclose(file);
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
    }
    rewind(file);
    if (errno) {
      fclose(file);
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
    }
    to_read = (size_t)fsize;
  }

  if (*buffer)
    to_read = sve4_min(to_read, *bufsize);

  // allocate buffer if needed
  if (*buffer == NULL) {
    *buffer = sve4_malloc(alloc, to_read);
    if (!*buffer) {
      fclose(file);
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    }
  }

  size_t nread = fread(*buffer, 1, to_read, file);
  int err = ferror(file);
  fclose(file);

  if (nread < to_read && err)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);

  *bufsize = nread;
  return sve4_decode_success;
}

typedef struct page_t {
  struct page_t* next;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  unsigned char data[(1 << 12) - 64 - sizeof(struct page_t*)];
} page_t;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static sve4_decode_error_t sve4_decode_read_file_ffmpeg(
    sve4_allocator_t* _Nullable alloc, char* _Nullable* _Nonnull buffer,
    size_t* _Nonnull bufsize, const char* _Nonnull url, size_t max_read) {
  sve4_decode_error_t err;
  AVIOContext* io_ctx = NULL;
  sve4_allocator_t arena = sve4_allocator_arena_init;
  int ret = avio_open2(&io_ctx, url, AVIO_FLAG_READ, NULL, NULL);
  if (ret < 0) {
    err = sve4_decode_ffmpegerr(ret);
    goto fail;
  }

  size_t to_read = max_read;
  if (*buffer)
    to_read = sve4_min(to_read, *bufsize);
  if (to_read == SIZE_MAX) {
    int64_t resource_size = avio_size(io_ctx);
    if (resource_size >= 0)
      to_read = (size_t)resource_size;
    // NOLINTNEXTLINE(misc-include-cleaner)
    else if (resource_size != AVERROR(ENOSYS)) {
      err = sve4_decode_ffmpegerr((int32_t)resource_size);
      goto fail;
    }
  }

  char* data = NULL;

  // ---------- Case 1: Known size (single allocation) ----------
  if (to_read != SIZE_MAX) {
    data = sve4_malloc(alloc, to_read);
    if (!data) {
      err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
      goto fail;
    }

    size_t total = 0;
    while (total < to_read) {
      int num_read = avio_read(io_ctx, (unsigned char*)data + total,
                               (int)(to_read - total));
      if (num_read == 0)
        break;
      if (num_read < 0) {
        err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
        goto fail;
      }
      total += (size_t)num_read;
    }

    *buffer = data;
    *bufsize = total;
    avio_closep(&io_ctx);
    return sve4_decode_success;
  }

  // ---------- Case 2: Unknown size (chunked read) ----------
  size_t total = 0;

  page_t* prev = NULL;
  page_t* first = NULL;
  bool eof = false;
  while (!eof) {
    page_t* chunk = sve4_calloc(&arena, sizeof(page_t));
    if (!chunk) {
      err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
      goto fail;
    }

    size_t num_read = 0;
    while (num_read < sizeof(chunk->data)) {
      int num_read_current_it =
          avio_read(io_ctx, &chunk->data[num_read],
                    (int)(sizeof(chunk->data) - num_read));
      if (num_read_current_it == 0) { // EOF
        eof = true;
        break;
      }
      if (num_read_current_it < 0) { // error
        err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
        goto fail;
      }
      num_read += (size_t)num_read_current_it;
    }

    if (prev)
      prev->next = chunk;
    prev = chunk;
    if (!first)
      first = chunk;

    total += num_read;

    if (max_read != SIZE_MAX && total > max_read) {
      total = max_read;
      break;
    }
  }

  // Combine all chunks into one contiguous buffer
  char* final_buf = sve4_malloc(alloc, total);
  if (!final_buf) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail;
  }

  size_t copied = 0;
  for (page_t* chunk = first; chunk; chunk = chunk->next) {
    size_t copy_len = sve4_min(total - copied, sizeof(chunk->data));
    memcpy(final_buf + copied, chunk->data, copy_len);
    copied += copy_len;
    if (copied >= total)
      break;
  }

  *buffer = final_buf;
  *bufsize = total;

  avio_closep(&io_ctx);
  sve4_allocator_arena_destroy(&arena);
  return sve4_decode_success;

fail:
  avio_closep(&io_ctx);
  sve4_allocator_arena_destroy(&arena);
  return err;
}

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_read_url(sve4_allocator_t* _Nullable alloc,
                                         char* _Nullable* _Nonnull buffer,
                                         size_t* _Nonnull bufsize,
                                         const char* _Nonnull url, bool binary,
                                         size_t max_read) {
#if SVE4_DECODE_HAVE_FFMPEG
  if (binary) {
    sve4_log_debug("Using ffmpeg to read binary url %s", url);
    return sve4_decode_read_file_ffmpeg(alloc, buffer, bufsize, url, max_read);
  }
#endif

  sve4_log_debug("Using stdio api to read binary url %s", url);
  return sve4_decode_read_file_stdio(alloc, buffer, bufsize, url, binary,
                                     max_read);
}
