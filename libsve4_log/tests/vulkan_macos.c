#include <stdio.h>
#include <stdlib.h>

#include "libsve4_utils/volk.h"

int main(void) {
  // Initialize Volk (loader)
  VkResult result = volkInitialize();
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to initialize Volk: %d\n", result);
    return 1;
  }

  // Create Vulkan instance
  VkInstance instance = VK_NULL_HANDLE;

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "volk_test",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "none",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames =
          (const char*[]){VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME},
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = NULL,
  };

  result = vkCreateInstance(&instance_info, NULL, &instance);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
    return 1;
  }

  printf("Created Vulkan instance: %p\n", (void*)instance);

  // Load instance-level Vulkan functions via Volk
  volkLoadInstance(instance);
  printf("Loaded Volk instance functions\n");

  // Destroy Vulkan instance
  vkDestroyInstance(instance, NULL);
  printf("Destroyed Vulkan instance: %p\n", (void*)instance);

  // Finalize Volk (optional)
  volkFinalize();

  return 0;
}
