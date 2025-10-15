#pragma once

#include <atomic>
#include <format>
#include <memory>
#include <string_view>
#include <thread>

#include "config.h"
#include "internal/message_payload.hpp"
#include "internal/mpsc_queue.hpp"

namespace log_library {

// Forward declarations
class Sink;

class Logger {
 public:
  static Logger &instance();

  void push_log(internal::MessagePayload &&payload);
  void shutdown();

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  ~Logger();

 private:
  Logger();
  void consumer_thread_loop();

  std::atomic<bool> m_done{false};
  alignas(64) std::atomic<uint64_t> m_signal{0};
  MPSCQueue<internal::MessagePayload, 1024> m_queue;
  std::jthread m_consumer_thread;
  std::unique_ptr<Sink> m_sink;
};


template <LogLevel level, typename... Args>
inline void log(std::format_string<Args...> fmt, Args &&...args) {
  if constexpr (level >= LOG_ACTIVE_LEVEL) {
    Logger::instance().push_log(internal::MessagePayload(
        level, fmt.get(), std::forward<Args>(args)...));
  }
}

template <typename... Args>
inline void log_debug(std::format_string<Args...> fmt, Args &&...args) {
  log<LOG_LEVEL_DEBUG>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_info(std::format_string<Args...> fmt, Args &&...args) {
  log<LOG_LEVEL_INFO>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_warn(std::format_string<Args...> fmt, Args &&...args) {
  log<LOG_LEVEL_WARN>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_error(std::format_string<Args...> fmt, Args &&...args) {
  log<LOG_LEVEL_ERROR>(fmt, std::forward<Args>(args)...);
}

}  // namespace log_library
