#pragma once

#include <log_library/file_sink_config.h>
#include <log_library/sink.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace log_library {

class WindowsFileSink : public Sink {
 public:
  explicit WindowsFileSink(const FileSinkConfig& config = {});
  ~WindowsFileSink() override;
  void write(const std::string& message, LogLevel level) override;
  void flush() override;

 private:
  FileSinkConfig config_;
#ifdef _WIN32
  HANDLE file_handle_;
#else
  void* file_handle_;
#endif
  size_t current_offset_;

  void initialize();
  bool create_file();
  bool rotate_file();
  void cleanup();
};

}  // namespace log_library

