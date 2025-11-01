cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(
    sve4
    VERSION 0.1.0
    DESCRIPTION "yet another one"
    HOMEPAGE_URL "https://github.com/btmxh/sve4"
    LANGUAGES
        C
        CXX
)

find_package(WebP)

find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")
set(CTEST_MEMORYCHECK_COMMAND valgrind)
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")

add_subdirectory(extern)

function(sve4_add_test)
    set(oneValueArgs
        PREFIX
        SOURCE
    )
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(
        PARSE_ARGV
        0
        ARG
        ""
        "${oneValueArgs}"
        "${multiValueArgs}"
    )

    if(NOT ARG_PREFIX)
        message(FATAL_ERROR "PREFIX is required")
    endif()
    if(NOT ARG_SOURCE)
        message(FATAL_ERROR "SOURCE is required")
    endif()

    get_filename_component(SOURCE_NAME ${ARG_SOURCE} NAME_WE)
    set(TEST_EXECUTABLE_NAME "test_${ARG_PREFIX}_${SOURCE_NAME}")

    add_executable(${TEST_EXECUTABLE_NAME} ${ARG_SOURCE})
    target_link_libraries(
        ${TEST_EXECUTABLE_NAME}
        PRIVATE
            ${ARG_LIBRARIES}
            munit
    )
    add_test(NAME ${TEST_EXECUTABLE_NAME} COMMAND ${TEST_EXECUTABLE_NAME})
endfunction()

function(sve4_set_target_default_properties)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "${multiValueArgs}")
    foreach(target IN LISTS ARG_TARGETS)
        if(SVE4_CLANG_TIDY)
            set(clangTidyArgs "clang-tidy;--use-color=1;--warnings-as-errors=*")
        endif()

        set_target_properties(
            ${target}
            PROPERTIES
                C_STANDARD
                    17
                C_STANDARD_REQUIRED
                    YES
                C_CLANG_TIDY
                    "${clangTidyArgs}"
                COMPILE_FLAGS
                    "${SVE4_C_FLAGS}"
        )
    endforeach()
endfunction()

add_subdirectory(libsve4_utils)
add_subdirectory(libsve4_decode)

if(NOT SVE4_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of sve4 only"
    )
endif()

include(cmake/dev-mode.cmake)
