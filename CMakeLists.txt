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

if(SVE4_DEVELOPER_MODE)
    if(NOT PROJECT_IS_TOP_LEVEL)
        message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of sve4 only"
        )
    endif()

    include(cmake/dev-mode.cmake)
endif()

find_package(WebP QUIET)

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
    sve4_set_target_default_properties(TARGETS ${TEST_EXECUTABLE_NAME})
endfunction()

function(sve4_set_target_default_properties)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "${multiValueArgs}")
    foreach(target IN LISTS ARG_TARGETS)
        set(clangTidyArgs "")
        if(SVE4_CLANG_TIDY)
            set(clangTidyArgs "clang-tidy;--use-color=1;--warnings-as-errors=*")
        endif()

        target_compile_definitions(
            ${target}
            PRIVATE
                "-DSVE4_ROOT_DIR=\"${CMAKE_CURRENT_LIST_DIR}\""
        )

        set_target_properties(
            ${target}
            PROPERTIES
                C_STANDARD
                    23
                C_STANDARD_REQUIRED
                    YES
                C_CLANG_TIDY
                    "${clangTidyArgs}"
                COMPILE_FLAGS
                    "${SVE4_C_FLAGS}"
        )
    endforeach()
endfunction()

include(GenerateExportHeader)
function(sve4_generate_export_header TARGET)
    include(GenerateExportHeader)

    # Ensure the output directory exists
    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/")
    file(MAKE_DIRECTORY "${_gen_dir}")

    string(TOUPPER ${TARGET} prefix)

    # Optional MSVC warning suppression (C4251)
    set(_pragma_suppress_c4251
        "/* Suppress C4251 only for MSVC */
#if defined(_MSC_VER) && !defined(__ICL)
#  define ${prefix}_SUPPRESS_C4251 _Pragma(\"warning(suppress:4251)\")
#else
#  define ${prefix}_SUPPRESS_C4251
#endif
"
    )

    generate_export_header(
        ${TARGET}
        BASE_NAME ${prefix}
        EXPORT_FILE_NAME "${_gen_dir}/${TARGET}_export.h"
        CUSTOM_CONTENT_FROM_VARIABLE _pragma_suppress_c4251
    )

    target_include_directories(${TARGET} PUBLIC "${_gen_dir}")
endfunction()

add_subdirectory(libsve4_utils)
add_subdirectory(libsve4_log)
add_subdirectory(libsve4_decode)
