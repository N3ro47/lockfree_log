# AddressSanitizer
add_executable(asan_test continous_running.cpp)
target_link_libraries(asan_test PRIVATE log_library::log_library)
target_compile_options(asan_test PRIVATE -fsanitize=address -g)
target_link_options(asan_test PRIVATE -fsanitize=address -g)

# LeakSanitizer
add_executable(lsan_test continous_running.cpp)
target_link_libraries(lsan_test PRIVATE log_library::log_library)
target_compile_options(lsan_test PRIVATE -fsanitize=leak -g)
target_link_options(lsan_test PRIVATE -fsanitize=leak -g)

# UndefinedBehaviorSanitizer
add_executable(ubsan_test continous_running.cpp)
target_link_libraries(ubsan_test PRIVATE log_library::log_library)
target_compile_options(ubsan_test PRIVATE -fsanitize=undefined -g)
target_link_options(ubsan_test PRIVATE -fsanitize=undefined -g)

# ThreadSanitizer
add_executable(tsan_test continous_running.cpp)
target_link_libraries(tsan_test PRIVATE log_library::log_library)
target_compile_options(tsan_test PRIVATE -fsanitize=thread -g)
target_link_options(tsan_test PRIVATE -fsanitize=thread -g)
