add_library(log_library_sinks "")  # Start with an empty library
add_library(log_library::sinks ALIAS log_library_sinks)

# Add source files common to all platforms
target_sources(log_library_sinks PRIVATE
    file_sink.cpp
    file_rotation.cpp
)

# Add platform-specific source files using generator expressions
# This is much cleaner than if/else blocks for sources.
if(WIN32)
    target_sources(log_library_sinks PRIVATE windows_file_sink.cpp)
else()
    target_sources(log_library_sinks PRIVATE linux_file_sink.cpp)
endif()

target_include_directories(log_library_sinks
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/sinks  # for private headers
)

target_compile_features(log_library_sinks PUBLIC cxx_std_23)

# The sinks component depends on the core component
target_link_libraries(log_library_sinks PUBLIC log_library::core)

