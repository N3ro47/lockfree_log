add_executable(simple_logger main.cpp)

target_link_libraries(simple_logger PRIVATE log_library::log_library)
