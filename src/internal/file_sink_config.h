#pragma once

#include <cstddef>
#include <string>

namespace log_library {

struct FileSinkConfig {
  std::string log_directory = "./logs/";

  size_t max_file_size = 256ULL * 1024 * 1024;

  size_t system_max_use = 2ULL * 1024 * 1024 * 1024;

  bool fsync_on_error = true;

  std::string base_filename = "app";

  std::string file_extension = ".log";
};

}  // namespace log_library
