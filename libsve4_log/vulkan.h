#pragma once

#include "sve4_log_export.h"

#include "libsve4_utils/volk.h"

SVE4_LOG_EXPORT
VKAPI_ATTR VkBool32 VKAPI_CALL sve4_log_vulkan_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
