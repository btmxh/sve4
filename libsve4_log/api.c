#include "api.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <libsve4_utils/defines.h>

void sve4__flog(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                sve4_log_level_t level, const char* _Nonnull fmt, ...) {
  va_list args;
  va_start(args, fmt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  sve4__flogv(log_id, file, line, level, fmt, args);
#pragma GCC diagnostic pop
  va_end(args);
}

void sve4__panic(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                 const char* _Nonnull fmt, ...) {
  va_list args;
  va_start(args, fmt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  sve4__flogv(log_id, file, line, SVE4_LOG_LEVEL_ERROR, fmt, args);
  va_end(args);
#pragma GCC diagnostic pop
  exit(1);
}

void sve4_glog(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
               bool endl, sve4_log_level_t level, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  sve4_glogv(log_id, file, line, endl, level, fmt, args);
  va_end(args);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
const char* sve4_log_shorten_path(char* buffer, size_t buf_size,
                                  const char* path,
                                  const char* trim_root_prefix) {
#ifdef _WIN32
  const char sep = '\\';
#else
  const char sep = '/';
#endif

  assert(buffer && path);

  if (trim_root_prefix && strlen(path) >= strlen(trim_root_prefix) &&
      memcmp(path, trim_root_prefix, strlen(trim_root_prefix)) == 0) {
    path += strlen(trim_root_prefix);
    if (*path == sep)
      ++path;
  }

  if (buf_size == 0)
    return path;

  // count number of `pathsep` in path, along with the strlen of path
  size_t pathlen = strlen(path);

  // number of written characters to buffer
  size_t idx = 0;

  // if absolute path then prepend a `/`, after that treat path like an ordinary
  // relative path
  if (*path == sep) {
    if (idx + 1 < buf_size)
      buffer[idx++] = sep;
    ++path;
  }

  // the filename should have at most `buf_size - 1` chars (including extension,
  // pathseps and whatnot)
  size_t bound = buf_size - 1;
  if (pathlen < bound)
    return path;

  size_t overbound = pathlen - bound;

  // iterate through all of the path elements
  const char* path_ptr = path;
  const char* path_end = path + pathlen;
  do {
    // e.g. /home/user/a.c\0
    //       ^ here is p
    //                    ^ here is path_end
    // after processing the first element...
    //                v this is next_sep
    // e.g. /home/user/a.c\0
    //            ^ here is p
    const char* next_sep = memchr(path_ptr, sep, (size_t)(path_end - path_ptr));
    next_sep = next_sep ? next_sep : path_end;
    size_t elem_size = (size_t)(next_sep - path_ptr);

    if (elem_size > 0) {
      size_t num_trim_chars = sve4_min(elem_size - 1, overbound);
      if (next_sep + 1 >= path_end) {
        num_trim_chars = 0;
      }

      assert(num_trim_chars <= elem_size && num_trim_chars <= overbound);
      elem_size -= num_trim_chars;
      overbound -= num_trim_chars;

      // copy the path element to the buffer
      for (size_t i = 0; i < elem_size && idx + 1 < buf_size; ++i) {
        buffer[idx++] = path_ptr[i];
      }
    }

    path_ptr = next_sep + 1;
    if (path_ptr < path_end && idx + 1 < buf_size) {
      buffer[idx++] = sep;
    }
  } while (path_ptr < path_end);

  assert(idx < buf_size);
  buffer[idx] = '\0';
  return buffer;
}
