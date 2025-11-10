#pragma once

#include <log_library/file_sink_config.h>
#include <log_library/sink.h>

namespace log_library {

class LinuxFileSink : public Sink {
 public:
  explicit LinuxFileSink(const FileSinkConfig& config = {});
  ~LinuxFileSink() override;
  void write(const std::string& message, LogLevel level) override;
  void flush() override;

 private:
  FileSinkConfig config_;
  int fd_;
  void* mapped_memory_;
  size_t current_offset_;

  void initialize();
  bool create_and_map_file();
  bool rotate_file();
  void sync_to_disk();
  void cleanup();
};

}  // namespace log_library