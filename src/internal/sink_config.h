#pragma once

// Users can override these defaults by defining them before including logger.h

#ifndef LOG_SINK_TYPE
#ifdef _WIN32
#define LOG_SINK_TYPE WindowsFileSink
#else
#define LOG_SINK_TYPE LinuxFileSink
#endif
#endif

namespace log_library {
class LinuxFileSink;
class WindowsFileSink;
}  // namespace log_library
