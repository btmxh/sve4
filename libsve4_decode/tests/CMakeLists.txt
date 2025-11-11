if(WebP_FOUND)
    sve4_add_test(PREFIX decode SOURCE webp.c LIBRARIES sve4::decode)
endif()

if(FFmpeg_AVFORMAT_FOUND AND FFmpeg_AVCODEC_FOUND AND FFmpeg_AVUTIL_FOUND)
    sve4_add_test(
        PREFIX decode
        SOURCE ffmpeg_packet_queue.c
        LIBRARIES
            sve4::decode
            FFmpeg::AVCODEC
            tinycthread
    )
endif()

sve4_add_test(PREFIX decode SOURCE generic.c LIBRARIES sve4::decode)
sve4_add_test(
    PREFIX decode
    SOURCE read.c
    LIBRARIES
        sve4::decode
        civetweb-c-library
        tinycthread
)
