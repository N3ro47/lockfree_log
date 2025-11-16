# Add a helper function at the top to reduce boilerplate
function(add_sanitizer_test name source_file)
  set(options)
  set(one_value_args)
  set(multi_value_args SANITIZERS)
  cmake_parse_arguments(AST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  add_executable(${name} ${source_file})
  target_link_libraries(${name} PRIVATE log_library::log_library)
  
  foreach(sanitizer ${AST_SANITIZERS})
    target_compile_options(${name} PRIVATE -fsanitize=${sanitizer} -g)
    target_link_options(${name} PRIVATE -fsanitize=${sanitizer} -g)
  endforeach()
endfunction()

# Now the definitions are much cleaner
add_sanitizer_test(asan_test continous_running.cpp SANITIZERS address)
add_sanitizer_test(lsan_test continous_running.cpp SANITIZERS leak)
add_sanitizer_test(ubsan_test continous_running.cpp SANITIZERS undefined)
add_sanitizer_test(tsan_test continous_running.cpp SANITIZERS thread)

add_executable(burst_consistency_test burst_consistency_test.cpp)
target_link_libraries(burst_consistency_test PRIVATE log_library::log_library)
target_compile_options(burst_consistency_test PRIVATE -fsanitize=thread -g)
target_link_options(burst_consistency_test PRIVATE -fsanitize=thread -g)

add_executable(thrughput_test logger_throughput.cpp)
target_link_libraries(thrughput_test PRIVATE log_library::log_library)
target_compile_options(thrughput_test PRIVATE -O3)
target_link_options(thrughput_test PRIVATE -O3)

add_executable(hot_path_latency hot_path_latency_test.cpp)
target_link_libraries(hot_path_latency PRIVATE log_library::log_library)
target_compile_options(hot_path_latency PRIVATE -O3)
target_link_options(hot_path_latency PRIVATE -O3)