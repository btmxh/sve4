set(SVE4_LOG_FILES
    ansi.h
    api.h
    api.c
    error.h
    error.c
    init.h
    init.c
    tty.h
    tty.c
    glfw.h
    glfw.c
)

if(FFmpeg_AVUTIL_FOUND)
    list(
        APPEND
        SVE4_LOG_FILES
        ffmpeg.h
        ffmpeg.c
    )
endif()

if(Vulkan_FOUND)
    list(
        APPEND
        SVE4_LOG_FILES
        vulkan.h
        vulkan.c
    )
endif()

if(BUILD_TESTING)
    list(
        APPEND
        SVE4_LOG_FILES
        init_test.h
        init_test.c
    )
endif()

add_library(sve4_log ${SVE4_LOG_FILES})
sve4_set_target_default_properties(TARGETS sve4_log)
sve4_generate_export_header(sve4_log)
add_library(sve4::log ALIAS sve4_log)
target_compile_definitions(
    sve4_log
    PRIVATE
        "SVE4_LOG_ID_MAIN=SVE4_LOG_ID_DEFAULT_SVE4_LOG"
)

target_link_libraries(
    sve4_log
    PUBLIC
        sve4_utils
        tinycthread
)

if(FFmpeg_AVUTIL_FOUND)
    message(STATUS "FFmpeg found, enabling FFmpeg support in sve4_log")
    target_link_libraries(sve4_log PUBLIC FFmpeg::AVUTIL)
    target_compile_definitions(sve4_log PUBLIC "SVE4_LOG_HAVE_FFMPEG")
endif()

if(Vulkan_FOUND)
    message(STATUS "Vulkan found, enabling Vulkan support in sve4_log")
    target_link_libraries(
        sve4_log
        PUBLIC
            Vulkan::Headers
            Vulkan::volk
    )
    target_compile_definitions(sve4_log PUBLIC "SVE4_LOG_HAVE_VULKAN")
endif()

if(glfw3_FOUND)
    message(STATUS "glfw3 found, enabling GLFW support in sve4_log")
    target_link_libraries(sve4_log PUBLIC glfw)
    target_compile_definitions(sve4_log PUBLIC "SVE4_LOG_HAVE_GLFW")
endif()

if(BUILD_TESTING)
    message(STATUS "munit found, enabling munit support in sve4_log")
    target_link_libraries(sve4_log PRIVATE munit)
    target_compile_definitions(sve4_log PUBLIC "SVE4_LOG_HAVE_MUNIT")
    add_subdirectory(tests)
endif()

if(SVE4_ENABLE_FUZZING)
    add_subdirectory(fuzz)
endif()
