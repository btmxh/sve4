#include "init.h"

#include <stdlib.h>

#define __STDC_WANT_LIB_EXT1__ 1
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tinycthread.h>

#include "allocator.h"
#include "ansi.h"
#include "api.h"
#include "buffer.h"
#include "error.h"
#include "tty.h"

#ifdef _WIN32
#define localtime_r(t, tm) localtime_s(tm, t)
#endif

#ifdef SVE4_HAS_MUNIT
#include <munit.h>
#endif

sve4_log_callback_t sve4_log_callback_ref(sve4_log_callback_t src) {
  return (sve4_log_callback_t){
      .callback = src.callback,
      .user_data = sve4_buffer_ref(src.user_data),
  };
}

void sve4_log_callback_free(sve4_log_callback_t* _Nullable callback) {
  if (callback)
    sve4_buffer_free(&callback->user_data);
}

sve4_log_shorten_path_config_t
sve4_log_shorten_path_config_ref(sve4_log_shorten_path_config_t src) {
  return (sve4_log_shorten_path_config_t){
      .max_length = src.max_length,
      .root_prefix = src.root_prefix,
      .user_data = sve4_buffer_ref(src.user_data),
  };
}

void sve4_shorten_path_config_free(
    sve4_log_shorten_path_config_t* _Nullable config) {
  if (config)
    sve4_buffer_free(&config->user_data);
}

sve4_log_id_mapping_t sve4_log_id_mapping_ref(sve4_log_id_mapping_t src) {
  return (sve4_log_id_mapping_t){
      .get_log_id_name = src.get_log_id_name,
      .user_data = sve4_buffer_ref(src.user_data),
  };
}

static const char* _Nullable get_log_id_name_default(
    sve4_log_id_t log_id, sve4_buffer_ref_t _Nullable user_data) {
  (void)user_data;
  switch (log_id) {
  case SVE4_LOG_ID_APPLICATION:
    return "application";
  case SVE4_LOG_ID_DEFAULT_SVE4_DECODE:
    return "sve4-decode";
  case SVE4_LOG_ID_DEFAULT_SVE4_LOG:
    return "sve4-log";
  case SVE4_LOG_ID_DEFAULT_FFMPEG:
    return "ffmpeg";
  case SVE4_LOG_ID_DEFAULT_VULKAN:
    return "vulkan";
  }

  return NULL;
}

sve4_log_id_mapping_t sve4_log_id_mapping_default(void) {
  return (sve4_log_id_mapping_t){
      .user_data = NULL,
      .get_log_id_name = get_log_id_name_default,
  };
}

void sve4_log_id_mapping_free(sve4_log_id_mapping_t* _Nullable mapping) {
  if (mapping)
    sve4_buffer_free(&mapping->user_data);
}

struct sve4_log_inner_t {
  sve4_log_config_t config;
  sve4_log_t _Nullable prev, next;
};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTNEXTLINE(misc-include-cleaner)
static mtx_t log_mutex;
static sve4_allocator_t* _Nullable log_allocator = NULL;
static sve4_log_t _Nullable log_first = NULL, log_last = NULL;
static bool stderr_ansi_supported = false;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

sve4_log_config_t sve4_log_config_ref(const sve4_log_config_t* src) {
  return (sve4_log_config_t){
      .level = src->level,
      .callback = sve4_log_callback_ref(src->callback),
      .id_mapping = sve4_log_id_mapping_ref(src->id_mapping),
      .path_shorten = sve4_log_shorten_path_config_ref(src->path_shorten),
  };
}

sve4_log_error_t sve4_log_init(sve4_allocator_t* allocator) {
  log_allocator = allocator;
  // NOLINTNEXTLINE(misc-include-cleaner)
  int err = mtx_init(&log_mutex, mtx_plain);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_APPLICATION, SVE4_LOG_LEVEL_ERROR,
              "unable to initialize mutex: %d", err);
    return SVE4_LOG_ERROR_THREADS;
  }

  return SVE4_LOG_ERROR_SUCCESS;
}

void sve4_log_destroy(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  while (log_first)
    sve4_log_remove_log(log_first);
#pragma GCC diagnostic pop
  // NOLINTNEXTLINE(misc-include-cleaner)
  mtx_destroy(&log_mutex);
}

sve4_log_error_t sve4_log_add_config(sve4_log_config_t* _Nonnull config,
                                     sve4_log_t* _Nullable log) {
  sve4_log_t log_handle =
      sve4_malloc(log_allocator, sizeof(struct sve4_log_inner_t));
  if (!log_handle)
    return SVE4_LOG_ERROR_MEMORY;
  memcpy(&log_handle->config, config, sizeof *config);
  memset(config, 0, sizeof *config);

  log_handle->prev = log_last;
  log_handle->next = NULL;

  // NOLINTNEXTLINE(misc-include-cleaner)
  int err = mtx_lock(&log_mutex);
  // NOLINTNEXTLINE(misc-include-cleaner)
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_ERROR,
              "unable to lock mutex: %d", err);
    sve4_free(log_allocator, log_handle);
    return SVE4_LOG_ERROR_THREADS;
  }

  log_last ? (log_last->next = log_handle) : (log_first = log_handle);
  log_last = log_handle;

  // NOLINTNEXTLINE(misc-include-cleaner)
  err = mtx_unlock(&log_mutex);
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_ERROR,
              "unable to unlock mutex, silently failing: %d", err);
  }

  if (log)
    *log = log_handle;
  return SVE4_LOG_ERROR_SUCCESS;
}

sve4_log_error_t sve4_log_remove_log(sve4_log_t _Nonnull log) {
  assert(log);
  int err = mtx_lock(&log_mutex);
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_ERROR,
              "unable to lock mutex: %d", err);
    return SVE4_LOG_ERROR_THREADS;
  }

  sve4_log_callback_free(&log->config.callback);
  sve4_shorten_path_config_free(&log->config.path_shorten);
  sve4_log_id_mapping_free(&log->config.id_mapping);

  log->prev ? (log->prev->next = log->next) : (log_first = log->next);
  log->next ? (log->next->prev = log->prev) : (log_last = log->prev);

  sve4_free(log_allocator, log);

  err = mtx_unlock(&log_mutex);
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_APPLICATION, SVE4_LOG_LEVEL_ERROR,
              "unable to unlock mutex, silently failing: %d", err);
  }

  return SVE4_LOG_ERROR_SUCCESS;
}

typedef struct {
  FILE* file;
  bool close;
  bool ansi;
} log_file_t;

static void log_file_free(char* _Nonnull data) {
  log_file_t* log_file = (log_file_t*)(void*)data;
  if (log_file->close && log_file->file)
    fclose(log_file->file);
}

static void log_to_file(sve4_log_record_t* _Nonnull record,
                        const sve4_log_config_t* _Nullable config,
                        FILE* _Nonnull out, bool flog, bool ansi) {
  static const char* log_level_names[] = {
      [SVE4_LOG_LEVEL_DEBUG] = "DEBUG",
      [SVE4_LOG_LEVEL_INFO] = "INFO",
      [SVE4_LOG_LEVEL_WARNING] = "WARN",
      [SVE4_LOG_LEVEL_ERROR] = "ERROR",
  };
  static const char* log_level_colors[] = {
      [SVE4_LOG_LEVEL_DEBUG] = SVE4_LOG_ANSI_FG_BRIGHT_BLUE,
      [SVE4_LOG_LEVEL_INFO] = SVE4_LOG_ANSI_FG_BRIGHT_GREEN,
      [SVE4_LOG_LEVEL_WARNING] = SVE4_LOG_ANSI_FG_BRIGHT_YELLOW,
      [SVE4_LOG_LEVEL_ERROR] = SVE4_LOG_ANSI_FG_BRIGHT_RED,
  };
#define TIMESTAMP_BUF_MAX_SIZE 16
  char timestamp_buf[TIMESTAMP_BUF_MAX_SIZE] = {0};
  strftime(timestamp_buf, sizeof(timestamp_buf), "%H:%M:%S", record->timestamp);

  const char* ansi_timestamp = "";
  const char* ansi_level = "";
  const char* ansi_location = "";
  const char* ansi_flog = "";
  const char* ansi_reset = "";
  const char* flog_text = "";

  if (ansi) {
    ansi_timestamp = SVE4_LOG_ANSI_FG_BRIGHT_BLACK;
    ansi_level = record->level >= 0 && record->level < SVE4_LOG_LEVEL_MAX
                     ? log_level_colors[record->level]
                     : SVE4_LOG_ANSI_FG_BLACK;
    ansi_location = SVE4_LOG_ANSI_FG_BRIGHT_BLACK;
    ansi_flog = SVE4_LOG_ANSI_FG_BRIGHT_BLACK;
    ansi_reset = SVE4_LOG_ANSI_RESET;
  }

  if (flog)
    flog_text = "[flog] ";

  char file_buf[SVE4_LOG_SHORTEN_PATH_MAX_LENGTH + 1] = {0};
  const char* file = sve4_log_shorten_path(
      file_buf, config ? config->path_shorten.max_length + 1 : 0, record->file,
      config ? config->path_shorten.root_prefix : SVE4_ROOT_DIR);
  fprintf(out, "%s%s %s%-5s %s%s%s:%zu:%s %s%s", ansi_timestamp, timestamp_buf,
          ansi_level,
          record->level >= 0 && record->level < SVE4_LOG_LEVEL_MAX
              ? log_level_names[record->level]
              : "???",
          ansi_reset, ansi_location, file, record->line, ansi_flog, flog_text,
          ansi_reset);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  vfprintf(out, record->msg, record->args);
#pragma GCC diagnostic pop
  if (record->endl) {
    fputc('\n', out);
  }

  // flush after important messages
  if (record->level >= SVE4_LOG_LEVEL_WARNING) {
    fflush(out);
  }
}

static void log_file_callback(sve4_log_record_t* _Nonnull record,
                              const sve4_log_config_t* conf) {
  assert(conf->callback.user_data);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  log_file_t* log_file =
      (log_file_t*)sve4_buffer_get_data(conf->callback.user_data);
#pragma GCC diagnostic pop
  log_to_file(record, conf, log_file->file, false, log_file->ansi);
}

sve4_log_error_t sve4_log_to_file(sve4_log_callback_t* callback, FILE* file,
                                  bool close, bool ansi) {
  callback->user_data =
      sve4_buffer_create(log_allocator, sizeof(log_file_t), log_file_free);
  if (!callback->user_data)
    return SVE4_LOG_ERROR_MEMORY;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnullable-to-nonnull-conversion"
  log_file_t* log_file = (log_file_t*)sve4_buffer_get_data(callback->user_data);
#pragma GCC diagnostic pop

  log_file->file = file;
  log_file->ansi = ansi;
  log_file->close = close;
  callback->callback = log_file_callback;
  return SVE4_LOG_ERROR_SUCCESS;
}

static void enable_stderr_ansi_escape_codes(void) {
  stderr_ansi_supported = sve4_log_enable_ansi_escape_codes(stderr);
}

sve4_log_error_t sve4_log_to_stderr(sve4_log_callback_t* callback,
                                    bool force_ansi) {
  // NOLINTNEXTLINE(misc-include-cleaner)
  static once_flag once;
  // NOLINTNEXTLINE(misc-include-cleaner)
  call_once(&once, enable_stderr_ansi_escape_codes);
  return sve4_log_to_file(callback, stderr, false,
                          force_ansi || stderr_ansi_supported);
}

void sve4_glogv(sve4_log_id_t log_id, const char* _Nonnull file, size_t line,
                bool endl, sve4_log_level_t level, const char* _Nonnull fmt,
                va_list args) {
  struct timespec log_timestamp;
  timespec_get(&log_timestamp, TIME_UTC);

  int err = 0;
  err = mtx_lock(&log_mutex);
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_ERROR,
              "unable to lock mutex: %d", err);
    return;
  }

  // NOTE: this is not thread-safe
  struct tm* timestamp = localtime(&(time_t){log_timestamp.tv_sec});

  for (sve4_log_t log = log_first; log; log = log->next) {
    sve4_log_record_t record = {
        .id = log_id,
        .level = level,
        .msg = fmt,
        .timestamp = timestamp,
        .fractional_timestamp = (int32_t)log_timestamp.tv_nsec,
        .file = file,
        .line = line,
        .endl = endl,
    };
    if (log->config.level > level)
      continue;

    va_copy(record.args, args);
    log->config.callback.callback(&record, &log->config);
    va_end(record.args);
  }

  err = mtx_unlock(&log_mutex);
  if (err != thrd_success) {
    sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_ERROR,
              "unable to unlock mutex, silently failing: %d", err);
  }
}

void sve4__flogv(sve4_log_id_t log_id, const char* file, size_t line,
                 sve4_log_level_t level, const char* fmt, va_list args) {
  struct timespec log_timestamp;
  timespec_get(&log_timestamp, TIME_UTC);
  struct tm timestamp = {0};
  localtime_r(&(time_t){log_timestamp.tv_sec}, &timestamp);

  sve4_log_record_t record = {
      .id = log_id,
      .level = level,
      .msg = fmt,
      .timestamp = &timestamp,
      .fractional_timestamp = (int32_t)log_timestamp.tv_nsec,
      .file = file,
      .line = line,
      .endl = true,
  };
  va_copy(record.args, args);
  log_to_file(&record, NULL, stderr, true, false);
  va_end(args);
}

#ifdef SVE4_HAS_MUNIT
static void log_munit_callback(sve4_log_record_t* _Nonnull record,
                               const sve4_log_config_t* _Nullable conf) {
  (void)conf;
  static const MunitLogLevel sve4_to_munit_log_level[] = {
      [SVE4_LOG_LEVEL_DEBUG] = MUNIT_LOG_DEBUG,
      [SVE4_LOG_LEVEL_INFO] = MUNIT_LOG_INFO,
      [SVE4_LOG_LEVEL_WARNING] = MUNIT_LOG_WARNING,
      [SVE4_LOG_LEVEL_ERROR] = MUNIT_LOG_ERROR,
  };

  va_list args_copy;
  va_copy(args_copy, record->args);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  int len = vsnprintf(NULL, 0, record->msg, args_copy);
#pragma GCC diagnostic pop
  va_end(args_copy);

  if (len < 0) {
    sve4_panic("vsnprintf failed, returning %d", len);
  }

  char* buf = sve4_calloc(log_allocator, (size_t)len + 1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  vsnprintf(buf, (size_t)len + 1, record->msg, record->args);
#pragma GCC diagnostic pop
  munit_logf_ex(sve4_to_munit_log_level[record->level], record->file,
                (int)record->line, "%s", buf);
  sve4_free(log_allocator, buf);
}

sve4_log_error_t sve4_log_to_munit(sve4_log_callback_t* _Nonnull callback) {
  callback->callback = log_munit_callback;
  callback->user_data = NULL;
  return SVE4_LOG_ERROR_SUCCESS;
}
#endif
