set(LOG_LIBRARY_SOURCES 
    logger.cpp
    file_rotation.cpp
)

find_package(Threads REQUIRED)

if(WIN32)
    list(APPEND LOG_LIBRARY_SOURCES windows_file_sink.cpp)
elseif(UNIX)
    list(APPEND LOG_LIBRARY_SOURCES linux_file_sink.cpp)
endif()

add_library(log_library ${LOG_LIBRARY_SOURCES})

add_library(log_library::log_library ALIAS log_library)

target_include_directories(log_library
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(log_library PUBLIC cxx_std_23)

target_link_libraries(log_library PRIVATE Threads::Threads)