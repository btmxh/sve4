#include "vulkan.h"

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include "api.h"

VKAPI_PTR VkBool32 VKAPI_CALL sve4_log_vulkan_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
  (void)user_data;

  sve4_log_level_t level = SVE4_LOG_LEVEL_INFO;
  switch (message_severity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    level = SVE4_LOG_LEVEL_DEBUG;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    level = SVE4_LOG_LEVEL_INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    level = SVE4_LOG_LEVEL_WARNING;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    level = SVE4_LOG_LEVEL_ERROR;
    break;
  default:
    break;
  }

  sve4_glog(SVE4_LOG_ID_DEFAULT_VULKAN, __FILE__, __LINE__, true, level,
            "(type: %d) %s\n", (int)message_types, callback_data->pMessage);

  (void)user_data;
  return VK_FALSE;
}
