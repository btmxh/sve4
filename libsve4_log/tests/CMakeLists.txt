sve4_add_test(PREFIX log SOURCE basic.c LIBRARIES sve4::log)
sve4_add_test(PREFIX log SOURCE error.c LIBRARIES sve4::log)
sve4_add_test(PREFIX log SOURCE custom.c LIBRARIES sve4::log)
sve4_add_test(PREFIX log SOURCE tty.c LIBRARIES sve4::log)

if(FFmpeg_FOUND)
    sve4_add_test(
        PREFIX log
        SOURCE ffmpeg.c
        LIBRARIES
            sve4::log
            FFmpeg::AVUTIL
    )
endif()

if(glfw3_FOUND)
    sve4_add_test(
        PREFIX log
        SOURCE glfw.c
        LIBRARIES
            sve4::log
            glfw
    )
endif()
