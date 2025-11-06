#pragma once

#include "api.h"
#include "munit.h"

static MunitResult test_app_log(const MunitParameter params[],
                                void* user_data) {
  (void)params;
  (void)user_data;
  int a = 621;
  double b = 14;
  long long c = 42;
  const char* msg = "%s %d";
  sve4_log_debug("Hi, a = %d, b = %lf", a, b);
  sve4_log_info("c = %lld, msg = \"%s\"", c, msg);
  sve4_log_warn("Hello, World!");

  sve4_glog(SVE4_LOG_ID_DEFAULT_FFMPEG, __FILE__, __LINE__, true,
            SVE4_LOG_LEVEL_INFO, "This is an FFmpeg log: %d", 456);
  sve4_glog(SVE4_LOG_ID_DEFAULT_VULKAN, __FILE__, __LINE__, true,
            SVE4_LOG_LEVEL_INFO, "This is an Vulkan log: %d", 456);
  sve4_glog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, __FILE__, __LINE__, true,
            SVE4_LOG_LEVEL_INFO, "This is an sve4-log log: %d", 456);
  sve4_glog(SVE4_LOG_ID_DEFAULT_SVE4_DECODE, __FILE__, __LINE__, true,
            SVE4_LOG_LEVEL_INFO, "This is an sve4-decode log: %d", 456);
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  sve4_glog(14, __FILE__, __LINE__, true, SVE4_LOG_LEVEL_INFO,
            "This is an 14 (peak) log: %d", 456);
  munit_log(MUNIT_LOG_INFO, "Hi");
  // munit panic on error
  // sve4_log_error("Error!");

  return MUNIT_OK;
}

static MunitResult test_external_log(const MunitParameter params[],
                                     void* user_data) {
  (void)params;
  (void)user_data;
  sve4_flog(SVE4_LOG_ID_DEFAULT_FFMPEG, SVE4_LOG_LEVEL_INFO,
            "This is an external log: %d", 456);
  sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_LOG, SVE4_LOG_LEVEL_INFO,
            "This is an sve4-log log: %d", 456);
  sve4_flog(SVE4_LOG_ID_DEFAULT_SVE4_DECODE, SVE4_LOG_LEVEL_INFO,
            "This is an sve4-decode log: %d", 456);
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  sve4_flog(42, SVE4_LOG_LEVEL_INFO, "This is an external log from 42: %d",
            727);
  sve4_glog(SVE4_LOG_ID_DEFAULT_VULKAN, "/usr/include/vulkan/vulkan.hpp", 621,
            true, SVE4_LOG_LEVEL_WARNING, "Vulkan validation layer warning: %s",
            "warning message");
  sve4_glog(SVE4_LOG_ID_DEFAULT_VULKAN,
            "C:\\VulkanSDK\\1.4.621.1\\Include\\vulkan\\vulkan.hpp", 14, true,
            SVE4_LOG_LEVEL_WARNING, "Vulkan validation layer warning: %s",
            "warning message (Windows version)");
  return MUNIT_OK;
}

static MunitResult test_freestanding_log(const MunitParameter params[],
                                         void* user_data) {
  (void)params;
  (void)user_data;
  sve4_flog(SVE4_LOG_ID_APPLICATION, SVE4_LOG_LEVEL_INFO,
            "This is a freestanding log: %d", 123);
  return MUNIT_OK;
}
