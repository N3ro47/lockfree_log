cmake_minimum_required(VERSION 3.20)
project(log_library VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

add_subdirectory(examples)

add_subdirectory(test)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS log_library
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
