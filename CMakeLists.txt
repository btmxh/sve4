cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(sve4 C CXX)

option(SVE4_BUILD_TESTS "Build sve4 tests" OFF)

find_package(WebP)

find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")
set(CTEST_MEMORYCHECK_COMMAND valgrind)
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")

add_subdirectory(extern)

function(sve4_add_test)
    set(oneValueArgs PREFIX SOURCE)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "${oneValueArgs}" "${multiValueArgs}")

    if(NOT ARG_PREFIX)
        message(FATAL_ERROR "PREFIX is required")
    endif()
    if(NOT ARG_SOURCE)
        message(FATAL_ERROR "SOURCE is required")
    endif()

    get_filename_component(SOURCE_NAME ${ARG_SOURCE} NAME_WE)
    set(TEST_EXECUTABLE_NAME "test_${ARG_PREFIX}_${SOURCE_NAME}")

    add_executable(${TEST_EXECUTABLE_NAME} ${ARG_SOURCE})
    target_link_libraries(${TEST_EXECUTABLE_NAME} PRIVATE ${ARG_LIBRARIES} munit)
    add_test(NAME ${TEST_EXECUTABLE_NAME} COMMAND ${TEST_EXECUTABLE_NAME})
endfunction()

if(SVE4_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(libsve4_utils)
add_subdirectory(libsve4_decode)
