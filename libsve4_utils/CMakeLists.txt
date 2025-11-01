add_library(
    sve4_utils
    allocator.h
    allocator.c
    buffer.h
    buffer.c
    formats.h
    formats.c
)
target_include_directories(sve4_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
sve4_set_target_default_properties(TARGETS sve4_utils)
add_library(sve4::utils ALIAS sve4_utils)

if(SVE4_BUILD_TESTS)
    add_subdirectory(tests)
endif()
