add_library(
    sve4_log
    ansi.h
    api.h
    api.c
    error.h
    error.c
    init.h
    init.c
    tty.h
    tty.c
)
target_include_directories(sve4_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
sve4_set_target_default_properties(TARGETS sve4_log)
sve4_generate_export_header(sve4_log)
add_library(sve4::log ALIAS sve4_log)

target_link_libraries(sve4_log PUBLIC sve4_utils)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
