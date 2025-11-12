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
          .pApplicationInfo =
              &(VkApplicationInfo){
                  .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                  .pApplicationName = "sve4_log_test",
                  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                  .pEngineName = "sve4_log",
                  .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                  .apiVersion = VK_API_VERSION_1_0,
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
