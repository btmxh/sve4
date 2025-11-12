set(sve4_utils_FILES
    allocator.h
    allocator.c
    buffer.h
    buffer.c
    formats.h
    formats.c
    arena.h
    arena.c
)

if(Vulkan_volk_FOUND)
    list(APPEND sve4_utils_FILES volk.h)
endif()

add_library(sve4_utils ${sve4_utils_FILES})
sve4_set_target_default_properties(TARGETS sve4_utils)
sve4_generate_export_header(sve4_utils)
add_library(sve4::utils ALIAS sve4_utils)
target_link_libraries(sve4_utils PRIVATE arena)

if(FFmpeg_AVUTIL_FOUND)
    message(STATUS "FFmpeg found, enabling FFmpeg support in sve4_utils")
    target_compile_definitions(sve4_utils PUBLIC SVE4_UTILS_HAVE_FFMPEG)
    target_link_libraries(sve4_utils PUBLIC FFmpeg::AVUTIL)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

include(CheckIncludeFile)
include(CMakePushCheckState)

if(Vulkan_volk_FOUND)
    message(STATUS "Volk found, enabling Volk support in sve4_utils")
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_INCLUDES ${Vulkan_INCLUDE_DIRS})
    check_include_file(
        volk.h
        SVE4_UTILS_VOLK_INCLUDE_VOLK_H
    )
    if(NOT SVE4_UTILS_VOLK_INCLUDE_VOLK_H)
        check_include_file(
            volk/volk.h
            SVE4_UTILS_VOLK_INCLUDE_VOLK_VOLK_H
        )
        if(NOT SVE4_UTILS_VOLK_INCLUDE_VOLK_VOLK_H)
            message(
                FATAL_ERROR
                "Could not find volk.h even though Volk was found by CMake."
            )
        endif()
    else()
        target_compile_definitions(
            sve4_utils
            PUBLIC
                SVE4_UTILS_VOLK_INCLUDE_VOLK_H
        )
    endif()
    cmake_pop_check_state()
    target_link_libraries(sve4_utils PUBLIC Vulkan::volk)
endif()
