if(WebP_FOUND)
    sve4_add_test(PREFIX decode SOURCE webp.c LIBRARIES sve4::decode)
endif()
