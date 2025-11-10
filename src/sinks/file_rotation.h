#pragma once

#include <string>
#include <vector>

#include <log_library/file_sink_config.h>

namespace log_library {

class FileRotationUtils {
 public:
  static std::string get_current_log_path(const FileSinkConfig& config);
  static std::string get_rotated_log_path(const FileSinkConfig& config,
                                          int rotation_number);
  static bool ensure_log_directory(const FileSinkConfig& config);
  static bool rotate_log_files(const FileSinkConfig& config);
  static size_t calculate_total_disk_usage(const FileSinkConfig& config);
  static void cleanup_old_files(const FileSinkConfig& config);
  static std::vector<std::string> get_rotated_files_sorted(
      const FileSinkConfig& config);
};

}  // namespace log_library
