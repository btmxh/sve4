#include "init_test.h"

#include <stdlib.h>
#include <string.h>

#include "libsve4_log/api.h"

#include "init.h"

#ifdef SVE4_LOG_HAVE_FFMPEG
#include "libsve4_log/ffmpeg.h"

#include <libavutil/log.h>
#endif

#ifdef SVE4_LOG_HAVE_GLFW
#include <GLFW/glfw3.h>

#include "glfw.h"
#endif

void sve4_log_test_setup(void) {
  if (sve4_log_init(NULL))
    sve4_panic("Failed to initialize sve4 logging");

  char* log_level_str = getenv("SVE4_LOG_LEVEL");
  sve4_log_level_t log_level = SVE4_LOG_LEVEL_DEBUG;
  if (log_level_str) {
    if (strcmp(log_level_str, "DEBUG") == 0)
      log_level = SVE4_LOG_LEVEL_DEBUG;
    else if (strcmp(log_level_str, "INFO") == 0)
      log_level = SVE4_LOG_LEVEL_INFO;
    else if (strcmp(log_level_str, "WARNING") == 0)
      log_level = SVE4_LOG_LEVEL_WARNING;
    else if (strcmp(log_level_str, "ERROR") == 0)
      log_level = SVE4_LOG_LEVEL_ERROR;
  }
  sve4_log_config_t conf = {
      .level = log_level,
      .path_shorten =
          {
              // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
              .max_length = 16,
              .root_prefix = SVE4_ROOT_DIR,
          },
      .id_mapping = sve4_log_id_mapping_default(),
  };

  sve4_log_config_t config = conf;
  if (sve4_log_to_stderr(&config.callback, true) ||
      sve4_log_add_config(&config, NULL))
    sve4_panic("Failed to set log to stderr");

#ifdef SVE4_LOG_HAVE_FFMPEG
  sve4_log_debug("Initializing FFmpeg logging integration");
  av_log_set_callback(sve4_log_ffmpeg_callback);
#endif

#ifdef SVE4_LOG_HAVE_GLFW
  sve4_log_debug("Initializing GLFW logging integration");
  glfwSetErrorCallback(sve4_log_glfw_callback);
#endif

#ifdef SVE4_LOG_HAVE_VULKAN
  sve4_log_debug("Vulkan logging integration is not automatically initialized "
                 "due to the reliance on Vulkan instance creation");
#endif
}

void sve4_log_test_teardown(void) {
  sve4_log_destroy();
}

#ifdef SVE4_LOG_HAVE_MUNIT
void* sve4_log_test_setup_func(const MunitParameter params[], void* user_data) {
  (void)params;
  (void)user_data;
  sve4_log_test_setup();
  return NULL;
}

void sve4_log_test_teardown_func(void* user_data) {
  (void)user_data;
  sve4_log_test_teardown();
}
#endif
