add_library(sve4_utils
		allocator.h
		allocator.c
		buffer.h
		buffer.c
		formats.h
		formats.c
)
target_include_directories(sve4_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(sve4::utils ALIAS sve4_utils)
set_target_properties(sve4_utils PROPERTIES C_STANDARD 17 C_STANDARD_REQUIRED YES C_CLANG_TIDY "clang-tidy;--use-color=1;-warnings-as-errors=*")

if(SVE4_BUILD_TESTS)
	add_subdirectory(tests)
endif()
