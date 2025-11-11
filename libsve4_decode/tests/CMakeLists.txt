if(WebP_FOUND)
    sve4_add_test(PREFIX decode SOURCE webp.c LIBRARIES sve4::decode)
endif()

sve4_add_test(PREFIX decode SOURCE generic.c LIBRARIES sve4::decode)
sve4_add_test(PREFIX decode SOURCE read.c LIBRARIES sve4::decode)
