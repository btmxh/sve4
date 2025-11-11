#include "read.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sve4_decode_export.h"

#include "libsve4_decode/error.h"
#include "libsve4_log/api.h"
#include "libsve4_utils/allocator.h"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "libsve4_utils/defines.h"

#ifdef SVE4_DECODE_HAVE_FFMPEG
#include "libsve4_utils/arena.h"

#include <libavformat/avio.h>
#include <libavutil/error.h>
#endif

static sve4_decode_error_t stdio_get_size(void* _Nonnull file,
                                          size_t* _Nonnull size) {
  if (fseek(file, 0, SEEK_END) != 0) {
#ifdef ESPIPE
    if (errno == ESPIPE)
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_UNIMPLEMENTED);
#endif
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
  }

  long fsize = ftell(file);
  if (fsize < 0) {
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
  }

  *size = (size_t)fsize;

  if (fseek(file, 0, SEEK_SET) != 0) {
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
  }

  return sve4_decode_success;
}

typedef struct page_t {
  struct page_t* next;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  unsigned char data[(1 << 12) - 64 - sizeof(struct page_t*)];
} page_t;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static sve4_decode_error_t sve4_decode_read_file_common(
    sve4_allocator_t* _Nullable alloc, char* _Nullable* _Nonnull buffer,
    size_t* _Nonnull bufsize, const char* _Nonnull url, bool binary,
    sve4_decode_error_t (*open_func)(const char* _Nonnull url, bool binary,
                                     void** _Nonnull file),
    void (*close_func)(void* _Nonnull file),
    sve4_decode_error_t (*get_size)(void* _Nonnull file, size_t* _Nonnull size),
    sve4_decode_error_t (*read_func)(void* _Nonnull file, char* _Nonnull buf,
                                     size_t to_read, size_t* _Nonnull nread)) {
  void* file = NULL;
  sve4_decode_error_t err = open_func(url, binary, &file);
  assert(file);
  if (!sve4_decode_error_is_success(err))
    return err;

  size_t to_read = *bufsize;
  if (to_read == SIZE_MAX) {
    err = get_size(file, &to_read);
    if (!sve4_decode_error_is_success(err)) {
      if (err.source != SVE4_DECODE_ERROR_SRC_DEFAULT ||
          err.error_code != SVE4_DECODE_ERROR_DEFAULT_UNIMPLEMENTED) {
        close_func(file);
        return err;
      }
    }
  }

  // if buffer is pre-allocated or size is available, then read in one go
  char* buf = *buffer;
  bool needs_alloc = !buf;
  if (buf || to_read < SIZE_MAX) {
    if (!buf)
      *buffer = buf = sve4_malloc(alloc, to_read);
    if (!buf) {
      close_func(file);
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    }

    err = read_func(file, buf, to_read, bufsize);
    if (!sve4_decode_error_is_success(err)) {
      close_func(file);
      if (needs_alloc)
        sve4_free(alloc, buf);
      return err;
    }

    close_func(file);
    return sve4_decode_success;
  }

  // otherwise, read in chunks and concatenate
  size_t final_page_size = 0;
  size_t total_size = 0;
  page_t* first_page = NULL;
  page_t* last_page = NULL;

  sve4_allocator_t arena = sve4_allocator_arena_init;
  sve4_allocator_impl_missing(&arena);

  while (true) {
    page_t* page = sve4_calloc(&arena, sizeof(page_t));
    if (!page) {
      err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
      goto fail_paged_read;
    }

    err = read_func(file, (char*)page->data, sizeof(page->data),
                    &final_page_size);
    if (err.source == SVE4_DECODE_ERROR_SRC_DEFAULT &&
        err.error_code == SVE4_DECODE_ERROR_DEFAULT_EOF) {
      err = sve4_decode_success;
      break;
    }

    if (!sve4_decode_error_is_success(err))
      goto fail_paged_read;

    last_page ? (last_page->next = page) : (first_page = page);
    last_page = page;
    total_size += final_page_size;
  }

  *buffer = sve4_malloc(alloc, total_size);
  if (!*buffer) {
    err = sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_MEMORY);
    goto fail_paged_read;
  }

  for (page_t* page = first_page; page; page = page->next) {
    size_t copy_size = page->next ? sizeof(page->data) : final_page_size;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(*buffer + (*bufsize), page->data, copy_size);
    *bufsize += copy_size;
  }

  assert(total_size == *bufsize);

fail_paged_read:
  close_func(file);
  sve4_allocator_arena_destroy(&arena);
  return err;
}

static sve4_decode_error_t stdio_open_func(const char* _Nonnull url,
                                           bool binary, void** _Nonnull file) {
  *file = fopen(url, binary ? "rb" : "r");
  if (!*file)
    return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
  return sve4_decode_success;
}

static void stdio_close_func(void* _Nonnull file) {
  fclose((FILE*)file);
}

static sve4_decode_error_t stdio_read_func(void* _Nonnull file,
                                           char* _Nonnull buf, size_t to_read,
                                           size_t* _Nonnull nread) {
  *nread = 0;
  while (*nread < to_read) {
    // NOLINTNEXTLINE(clang-analyzer-unix.Stream)
    size_t read_bytes = fread(buf + *nread, 1, to_read - *nread, (FILE*)file);
    if (read_bytes == 0) {
      if (ferror((FILE*)file))
        return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
      if (feof((FILE*)file))
        break;
    }
    *nread += read_bytes;
  }

  return sve4_decode_success;
}

static sve4_decode_error_t sve4_decode_read_file_stdio(
    sve4_allocator_t* _Nullable alloc, char* _Nullable* _Nonnull buffer,
    size_t* _Nonnull bufsize, const char* _Nonnull url, bool binary) {
#define FILE_PREFIX "file://"
#define FILE_PREFIX_LEN (sizeof(FILE_PREFIX) - 1)
  if (strncmp(url, FILE_PREFIX, FILE_PREFIX_LEN) == 0)
    url += FILE_PREFIX_LEN;

  return sve4_decode_read_file_common(alloc, buffer, bufsize, url, binary,
                                      stdio_open_func, stdio_close_func,
                                      stdio_get_size, stdio_read_func);
}

#ifdef SVE4_DECODE_HAVE_FFMPEG
static sve4_decode_error_t ffmpeg_open_func(const char* _Nonnull url,
                                            bool binary, void** _Nonnull file) {
  (void)binary;
  AVIOContext* io_ctx = NULL;
  int ret = avio_open2(&io_ctx, url, AVIO_FLAG_READ, NULL, NULL);
  if (ret < 0) {
    return sve4_decode_ffmpegerr(ret);
  }
  *file = io_ctx;
  return sve4_decode_success;
}

static void ffmpeg_close_func(void* _Nonnull file) {
  AVIOContext* io_ctx = (AVIOContext*)file;
  avio_closep(&io_ctx);
}

static sve4_decode_error_t ffmpeg_get_size(void* _Nonnull file,
                                           size_t* _Nonnull size) {
  AVIOContext* io_ctx = (AVIOContext*)file;
  int64_t resource_size = avio_size(io_ctx);
  if (resource_size < 0) {
    // NOLINTNEXTLINE(misc-include-cleaner)
    if (resource_size == AVERROR(ENOSYS)) {
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_UNIMPLEMENTED);
    }
    return sve4_decode_ffmpegerr((int32_t)resource_size);
  }
  *size = (size_t)resource_size;
  return sve4_decode_success;
}

static sve4_decode_error_t ffmpeg_read_func(void* _Nonnull file,
                                            char* _Nonnull buf, size_t to_read,
                                            size_t* _Nonnull nread) {
  AVIOContext* io_ctx = (AVIOContext*)file;
  size_t total = 0;
  while (total < to_read) {
    int num_read =
        avio_read(io_ctx, (unsigned char*)buf + total, (int)(to_read - total));
    if (num_read == AVERROR_EOF)
      break;
    if (num_read < 0) {
      return sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_IO);
    }
    total += (size_t)num_read;
  }
  *nread = total;
  return total > 0 ? sve4_decode_success
                   : sve4_decode_defaulterr(SVE4_DECODE_ERROR_DEFAULT_EOF);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static sve4_decode_error_t sve4_decode_read_file_ffmpeg(
    sve4_allocator_t* _Nullable alloc, char* _Nullable* _Nonnull buffer,
    size_t* _Nonnull bufsize, const char* _Nonnull url) {
  return sve4_decode_read_file_common(alloc, buffer, bufsize, url, true,
                                      ffmpeg_open_func, ffmpeg_close_func,
                                      ffmpeg_get_size, ffmpeg_read_func);
}
#endif

SVE4_DECODE_EXPORT
sve4_decode_error_t sve4_decode_read_url(sve4_allocator_t* _Nullable alloc,
                                         char* _Nullable* _Nonnull buffer,
                                         size_t* _Nonnull bufsize,
                                         const char* _Nonnull url,
                                         bool binary) {
#ifdef SVE4_DECODE_HAVE_FFMPEG
  if (binary) {
    sve4_log_debug("Using ffmpeg to read binary url %s", url);
    return sve4_decode_read_file_ffmpeg(alloc, buffer, bufsize, url);
  }
#endif

  sve4_log_debug("Using stdio api to read binary url %s", url);
  return sve4_decode_read_file_stdio(alloc, buffer, bufsize, url, binary);
}
