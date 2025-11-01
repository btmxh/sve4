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

include(GenerateExportHeader)
set(pragma_suppress_c4251
    "
/* This needs to suppress only for MSVC */
#if defined(_MSC_VER) && !defined(__ICL)
#  define SHARED_SUPPRESS_C4251 _Pragma(\"warning(suppress:4251)\")
#else
#  define SHARED_SUPPRESS_C4251
#endif
"
)
generate_export_header(
    sve4_utils
    BASE_NAME SVE4
    EXPORT_FILE_NAME
        ${CMAKE_CURRENT_BINARY_DIR}/generated/sve4_utils/sve4_export.h
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

target_include_directories(
    sve4_utils
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/generated/sve4_utils
)
add_library(sve4::utils ALIAS sve4_utils)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
