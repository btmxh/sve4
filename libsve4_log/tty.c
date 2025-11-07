#include "tty.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "libsve4_utils/defines.h"

// NOLINTNEXTLINE(misc-include-cleaner)
#include <tinycthread.h>

#if sve4_has_include(<unistd.h>)
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#include <windows.h>
#endif

typedef enum {
  TTY_IDX_STDIN = 0,
  TTY_IDX_STDOUT = 1,
  TTY_IDX_STDERR = 2,
  TTY_IDX_MAX = 3,
} tty_index_t;

static inline tty_index_t sve4_fileno(FILE* _Nonnull file) {
  if (file == stdin)
    return TTY_IDX_STDIN;
  if (file == stdout)
    return TTY_IDX_STDOUT;
  if (file == stderr)
    return TTY_IDX_STDERR;
  return TTY_IDX_MAX;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static bool isatty_result[TTY_IDX_MAX] = {0};

static void isatty_init(void) {
  for (int i = 0; i < TTY_IDX_MAX; i++) {
#if sve4_has_include(<unistd.h>)
    isatty_result[i] = isatty(i);
#elif defined(_WIN32)
    isatty_result[i] = _isatty(i);
#else
    isatty_result[i] = false;
#endif
  }
}

static inline bool sve4_isatty(FILE* _Nonnull file) {
  // NOLINTNEXTLINE(misc-include-cleaner)
  static once_flag once = ONCE_FLAG_INIT;
  // NOLINTNEXTLINE(misc-include-cleaner)
  call_once(&once, isatty_init);
  int file_no = (int)sve4_fileno(file);
  return file_no > 0 && file_no < TTY_IDX_MAX && isatty_result[file_no];
}

#ifdef _WIN32
HANDLE get_file_handle(FILE* file) {
  int fd = sve4_fileno(file);
  switch (fd) {
  case TTY_IDX_STDIN:
    return GetStdHandle(STD_INPUT_HANDLE);
  case TTY_IDX_STDOUT:
    return GetStdHandle(STD_OUTPUT_HANDLE);
  case TTY_IDX_STDERR:
    return GetStdHandle(STD_ERROR_HANDLE);
  }

  return NULL;
}
#endif

bool sve4_log_enable_ansi_escape_codes(FILE* file) {
#if defined(__unix__) || defined(__unix) ||                                    \
    defined(__APPLE__) && defined(__MACH__)
  (void)file;
  return sve4_isatty(file);
#else
  HANDLE handle = get_file_handle(file);
  if (handle == NULL || handle == INVALID_HANDLE_VALUE)
    return false;
  DWORD mode = 0;
  if (!GetConsoleMode(handle, &mode))
    return false;
  mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(handle, mode))
    return false;
  return true;
#endif
}
