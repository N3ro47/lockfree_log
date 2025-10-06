#pragma once

#include "config.h"
#include <format>
#include <string_view>
#include <memory>
#include "internal/message_payload.hpp"

namespace log_library {

class Logger {
public:
    static Logger& instance();

    void push_log(internal::MessagePayload&& payload);
    void shutdown();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    ~Logger();

private:
    Logger();

    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace log_library


#define LOG(level, fmt, ...)                                                   \
  do {                                                                         \
    if constexpr (level >= LOG_ACTIVE_LEVEL) {                                 \
      (void)std::format(fmt __VA_OPT__(, ) __VA_ARGS__);                       \
      log_library::Logger::instance().push_log(                                \
          log_library::internal::MessagePayload(level,                         \
                                           fmt __VA_OPT__(, ) __VA_ARGS__));   \
    }                                                                          \
  } while (false)


#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(LOG_LEVEL_INFO, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG(LOG_LEVEL_WARN, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, fmt __VA_OPT__(, ) __VA_ARGS__)