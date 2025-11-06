#pragma once

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

VKAPI_ATTR VkBool32 VKAPI_CALL sve4_log_vulkan_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
