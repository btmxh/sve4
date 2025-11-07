add_library(
    sve4_utils
    allocator.h
    allocator.c
    buffer.h
    buffer.c
    formats.h
    formats.c
    arena.h
    arena.c
)
target_include_directories(sve4_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
sve4_set_target_default_properties(TARGETS sve4_utils)
sve4_generate_export_header(sve4_utils)
add_library(sve4::utils ALIAS sve4_utils)
target_link_libraries(sve4_utils PRIVATE arena)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
