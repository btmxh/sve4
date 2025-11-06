add_executable(fuzz_shorten_path shorten_path.c)
target_link_libraries(fuzz_shorten_path PRIVATE sve4::log)
