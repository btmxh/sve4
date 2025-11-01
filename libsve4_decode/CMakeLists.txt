set(SVE4_DECODE_FILES
    ram_frame.h
    ram_frame.c
    frame.h
    frame.c
    error.h
)

if(WebP_FOUND)
    list(
        APPEND
        SVE4_DECODE_FILES
        libwebp.h
        libwebp.c
    )
endif()

add_library(sve4_decode ${SVE4_DECODE_FILES})
target_include_directories(sve4_decode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sve4_decode PUBLIC sve4_utils)
sve4_set_target_default_properties(TARGETS sve4_decode)

if(WebP_FOUND)
    target_link_libraries(
        sve4_decode
        PUBLIC
            WebP::webp
            WebP::webpdemux
    )
endif()

add_library(sve4::decode ALIAS sve4_decode)

if(SVE4_BUILD_TESTS)
    add_subdirectory(tests)
endif()
