#include <log_library/file_sink.h>

#include <memory>

#ifdef _WIN32
#include "../windows_file_sink.h"
#else
#include "../linux_file_sink.h"
#endif

namespace log_library {

std::unique_ptr<Sink> create_file_sink(const FileSinkConfig& config) {
#ifdef _WIN32
  return std::make_unique<WindowsFileSink>(config);
#else
  return std::make_unique<LinuxFileSink>(config);
#endif
}

}  // namespace log_library