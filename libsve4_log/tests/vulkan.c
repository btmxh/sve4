#include "libsve4_log/vulkan.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "libsve4_log/api.h"
#include "libsve4_log/init_test.h"
#include "libsve4_utils/volk.h"

#include "munit.h"

static MunitResult test_vulkan_basic(const MunitParameter params[],
                                     void* user_data) {
  (void)params;
  (void)user_data;

  VkResult result = volkInitialize();
  if (result != VK_SUCCESS)
    sve4_panic("Failed to initialize volk: %d", result);

  VkInstance instance = VK_NULL_HANDLE;

  result = vkCreateInstance(NULL, NULL, &instance);
  munit_assert_int(result, !=, VK_SUCCESS);

  instance = VK_NULL_HANDLE;
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
      .pfnUserCallback = sve4_log_vulkan_callback,
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
  munit_assert_int(result, ==, VK_SUCCESS);
  sve4_log_debug("Created Vulkan instance: %p", (void*)instance);

  volkLoadInstance(instance);
  sve4_log_debug("Loaded volk for instance: %p", (void*)instance);

  vkDestroyInstance(instance, NULL);
  sve4_log_debug("Destroyed Vulkan instance: %p", (void*)instance);

  volkFinalize();

  return MUNIT_OK;
}

static const MunitSuite test_suite = {
    "/vulkan",
    (MunitTest[]){
        {
            "/basic",
            test_vulkan_basic,
            NULL,
            NULL,
            MUNIT_TEST_OPTION_NONE,
            NULL,
        },
        {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE,
         NULL} /* Mark the end of the array */
    },
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  sve4_log_test_setup();
  int ret = munit_suite_main(&test_suite, NULL, argc, argv);
  sve4_log_test_teardown();
  return ret;
}
