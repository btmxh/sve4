#include <stdio.h>
#include <stdlib.h>

#include "libsve4_utils/volk.h"

// Debug callback for VK_EXT_debug_utils
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageTypes,
               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
               void* pUserData) {

  (void)messageTypes;
  (void)pUserData;

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    fprintf(stdout, "VERBOSE: %s\n", pCallbackData->pMessage);
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    fprintf(stdout, "INFO: %s\n", pCallbackData->pMessage);
  } else if (messageSeverity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    fprintf(stderr, "WARNING: %s\n", pCallbackData->pMessage);
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    fprintf(stderr, "ERROR: %s\n", pCallbackData->pMessage);
  }

  return VK_FALSE; // Do not abort Vulkan call
}

int main(void) {
  // Initialize Volk (loader)
  VkResult result = volkInitialize();
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to initialize Volk: %d\n", result);
    return 1;
  }

  VkInstance instance = VK_NULL_HANDLE;

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "volk_debug_pnext",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "none",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  // Prepare debug messenger create info to attach via pNext
  VkDebugUtilsMessengerCreateInfoEXT debug_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = NULL,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback,
      .pUserData = NULL,
  };

  const char* extensions[] = {VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                              VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = &debug_info, // <-- attach debug messenger via pNext
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = 2,
      .ppEnabledExtensionNames = extensions,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = NULL,
  };

  result = vkCreateInstance(&instance_info, NULL, &instance);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
    return 1;
  }

  printf("Created Vulkan instance with debug callback via pNext: %p\n",
         (void*)instance);

  volkLoadInstance(instance);
  printf("Loaded Vulkan instance functions via Volk\n");

  // Destroy Vulkan instance
  vkDestroyInstance(instance, NULL);
  printf("Destroyed Vulkan instance: %p\n", (void*)instance);

  volkFinalize();
  return 0;
}
