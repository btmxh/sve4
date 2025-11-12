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

  VkInstance instance;
  VkResult result = vkCreateInstance(NULL, NULL, &instance);
  munit_assert_int(result, !=, VK_SUCCESS);

  result = vkCreateInstance(
      &(VkInstanceCreateInfo){
          .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
          .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
          .pNext =
              &(VkDebugUtilsMessengerCreateInfoEXT){
                  .sType =
                      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                  .messageSeverity =
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                  .messageType =
                      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                      VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
                  .pfnUserCallback = sve4_log_vulkan_callback,
              },
          .pApplicationInfo =
              &(VkApplicationInfo){
                  .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                  .pApplicationName = "sve4_log_test",
                  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                  .pEngineName = "sve4_log",
                  .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                  .apiVersion = VK_API_VERSION_1_0,
              },
          .enabledExtensionCount = 1,
          .ppEnabledExtensionNames =
              (const char*[]){
                  VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
              },
      },
      NULL, &instance);
  munit_assert_int(result, ==, VK_SUCCESS);
  volkLoadInstance(instance);

  vkDestroyInstance(instance, NULL);

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
  VkResult result = volkInitialize();
  if (result != VK_SUCCESS)
    sve4_panic("Failed to initialize volk: %d", result);
  int ret = munit_suite_main(&test_suite, NULL, argc, argv);
  volkFinalize();
  sve4_log_test_teardown();
  return ret;
}
