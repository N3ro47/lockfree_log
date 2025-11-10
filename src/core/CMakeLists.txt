add_library(log_library_core logger.cpp)
add_library(log_library::core ALIAS log_library_core)

target_include_directories(log_library_core
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_compile_features(log_library_core PUBLIC cxx_std_23)

find_package(Threads REQUIRED)
target_link_libraries(log_library_core PRIVATE Threads::Threads)

