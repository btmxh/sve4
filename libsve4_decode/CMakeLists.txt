set(SVE4_DECODE_FILES
    ram_frame.h
    ram_frame.c
    frame.h
    frame.c
    decoder.h
    decoder.c
    error.h
    read.h
    read.c
)

if(WebP_FOUND)
    list(
        APPEND
        SVE4_DECODE_FILES
        libwebp.h
        libwebp.c
    )
endif()
if(FFmpeg_FOUND)
    list(
        APPEND
        SVE4_DECODE_FILES
        ffmpeg.h
        ffmpeg.c
        ffmpeg_demuxer.h
        ffmpeg_demuxer.c
        ffmpeg_demuxer_thread.h
        ffmpeg_demuxer_thread.c
        ffmpeg_decoder.h
        ffmpeg_decoder.c
        ffmpeg_packet_queue.h
        ffmpeg_packet_queue.c
    )
endif()

add_library(sve4_decode ${SVE4_DECODE_FILES})
target_link_libraries(
    sve4_decode
    PUBLIC
        sve4::utils
        sve4::log
)
sve4_set_target_default_properties(TARGETS sve4_decode)
sve4_generate_export_header(sve4_decode)
target_compile_definitions(
    sve4_decode
    PRIVATE
        "SVE4_LOG_ID_MAIN=SVE4_LOG_ID_DEFAULT_SVE4_DECODE"
)

if(WebP_FOUND)
    target_link_libraries(
        sve4_decode
        PUBLIC
            WebP::webp
            WebP::webpdemux
    )
    target_compile_definitions(sve4_decode PUBLIC SVE4_DECODE_HAVE_WEBP)
endif()

if(FFmpeg_FOUND)
    target_link_libraries(
        sve4_decode
        PUBLIC
            FFmpeg::AVCODEC
            FFmpeg::AVFORMAT
            FFmpeg::AVUTIL
    )
    target_compile_definitions(sve4_decode PUBLIC SVE4_DECODE_HAVE_FFMPEG)
endif()

add_library(sve4::decode ALIAS sve4_decode)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
