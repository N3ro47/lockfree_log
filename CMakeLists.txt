cmake_minimum_required(VERSION 3.20)
project(log_library VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add the main library components
add_subdirectory(src/core)
add_subdirectory(src/sinks)

# Create a single INTERFACE library to represent the whole package
# This makes linking for consumers (examples, tests) much easier.
add_library(log_library INTERFACE)
target_link_libraries(log_library INTERFACE log_library::core log_library::sinks)

# Make an alias for consistency
add_library(log_library::log_library ALIAS log_library)

add_subdirectory(examples)

add_subdirectory(test)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install all the component targets that make up the library
# Note: We install the actual targets, not the aliases
install(
    TARGETS log_library_core log_library_sinks
    EXPORT log_library-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT log_library-targets
    FILE log_library-targets.cmake
    NAMESPACE log_library::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/log_library
)

configure_package_config_file(
    "cmake/log_library-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/log_library-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/log_library"
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/log_library-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/log_library"
)
