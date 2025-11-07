#include "glfw.h"

#include "api.h"

void sve4_log_glfw_callback(int error_code, const char* description) {
  sve4_glog(SVE4_LOG_ID_DEFAULT_GLFW, __FILE__, __LINE__, true,
            SVE4_LOG_LEVEL_ERROR, "Error %d: %s", error_code, description);
}
