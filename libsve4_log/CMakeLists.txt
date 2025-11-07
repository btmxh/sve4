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

if(FFmpeg_FOUND)
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

add_library(sve4_log ${SVE4_LOG_FILES})

target_include_directories(sve4_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
sve4_set_target_default_properties(TARGETS sve4_log)
sve4_generate_export_header(sve4_log)
add_library(sve4::log ALIAS sve4_log)

target_link_libraries(
    sve4_log
    PUBLIC
        sve4_utils
        tinycthread
)

if(FFmpeg_FOUND)
    target_link_libraries(sve4_log PUBLIC FFmpeg::AVUTIL)
endif()

if(Vulkan_FOUND)
    target_link_libraries(
        sve4_log
        PUBLIC
            Vulkan::Headers
            Vulkan::volk
    )
    target_compile_definitions(sve4_log PUBLIC "SVE4_HAS_VULKAN")
endif()

if(BUILD_TESTING)
    target_link_libraries(sve4_log PRIVATE munit)
    target_compile_definitions(sve4_log PUBLIC "SVE4_HAS_MUNIT")
    add_subdirectory(tests)
endif()

if(SVE4_ENABLE_FUZZING)
    add_subdirectory(fuzz)
endif()
