#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main(void) {
  const char* path = getenv("PATH");
  printf("PATH=%s\n", path ? path : "(null)");
#ifdef _WIN32
  HMODULE vulkan_dll = LoadLibraryA("vulkan-1.dll");
  if (vulkan_dll == NULL) {
    printf("Failed to load vulkan-1.dll, error code %lu\n", GetLastError());
    system("dir C:\\VulkanSDK\\1.4.328.1\\Bin");
    return 1;
  }

  FreeLibrary(vulkan_dll);
#endif
}
