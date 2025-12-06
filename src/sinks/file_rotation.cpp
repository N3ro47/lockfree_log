#include "file_rotation.h"

#include <algorithm>
#include <filesystem>

namespace log_library {

std::string FileRotationUtils::get_current_log_path(
    const FileSinkConfig& config) {
  return config.log_directory + config.base_filename + config.file_extension;
}

std::string FileRotationUtils::get_rotated_log_path(
    const FileSinkConfig& config, int rotation_number) {
  return config.log_directory + config.base_filename + config.file_extension +
         "." + std::to_string(rotation_number);
}

bool FileRotationUtils::ensure_log_directory(const FileSinkConfig& config) {
  try {
    std::filesystem::create_directories(config.log_directory);
    return true;
  } catch (const std::filesystem::filesystem_error&) {
    return false;
  }
}

bool FileRotationUtils::rotate_log_files(const FileSinkConfig& config) {
  try {
    std::string current_path = get_current_log_path(config);

    int max_rotation = 0;
    for (int i = 1; i <= 100; ++i) {
      std::string rotated_path = get_rotated_log_path(config, i);
      if (std::filesystem::exists(rotated_path)) {
        max_rotation = i;
      } else {
        break;
      }
    }

    for (int i = max_rotation; i >= 1; --i) {
      std::string old_path = get_rotated_log_path(config, i);
      std::string new_path = get_rotated_log_path(config, i + 1);
      std::filesystem::rename(old_path, new_path);
    }

    if (std::filesystem::exists(current_path)) {
      std::string first_rotated = get_rotated_log_path(config, 1);
      std::filesystem::rename(current_path, first_rotated);
    }

    return true;
  } catch (const std::filesystem::filesystem_error&) {
    return false;
  }
}

size_t FileRotationUtils::calculate_total_disk_usage(
    const FileSinkConfig& config) {
  size_t total_size = 0;

  try {
    std::string current_path = get_current_log_path(config);
    if (std::filesystem::exists(current_path)) {
      total_size += std::filesystem::file_size(current_path);
    }

    for (int i = 1; i <= 100; ++i) {
      std::string rotated_path = get_rotated_log_path(config, i);
      if (std::filesystem::exists(rotated_path)) {
        total_size += std::filesystem::file_size(rotated_path);
      } else {
        break;
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
  }

  return total_size;
}

void FileRotationUtils::cleanup_old_files(const FileSinkConfig& config) {
  size_t current_usage = calculate_total_disk_usage(config);

  if (current_usage <= config.system_max_use) {
    return;
  }

  try {
    for (int i = 1; i <= 100; ++i) {
      std::string rotated_path = get_rotated_log_path(config, i);
      if (std::filesystem::exists(rotated_path)) {
        std::filesystem::remove(rotated_path);
        current_usage = calculate_total_disk_usage(config);

        if (current_usage <= config.system_max_use) {
          break;
        }
      } else {
        break;
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
  }
}

std::vector<std::string> FileRotationUtils::get_rotated_files_sorted(
    const FileSinkConfig& config) {
  std::vector<std::pair<std::string, std::filesystem::file_time_type>> files;

  try {
    for (int i = 1; i <= 100; ++i) {
      std::string rotated_path = get_rotated_log_path(config, i);
      if (std::filesystem::exists(rotated_path)) {
        auto mtime = std::filesystem::last_write_time(rotated_path);
        files.emplace_back(rotated_path, mtime);
      } else {
        break;
      }
    }

    std::sort(files.begin(), files.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    std::vector<std::string> result;
    for (const auto& file : files) {
      result.push_back(file.first);
    }
    return result;
  } catch (const std::filesystem::filesystem_error&) {
    return {};
  }
}

}  // namespace log_library
