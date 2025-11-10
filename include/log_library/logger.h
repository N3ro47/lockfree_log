#pragma once

#include <atomic>
#include <format>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#include "config.h"
#include "internal/message_payload.hpp"
#include "internal/mpsc_queue.hpp"
#include "sink.h"

namespace log_library {

class Logger {
 public:
  explicit Logger(std::vector<std::unique_ptr<Sink>> sinks);

  template <typename... Args>
  void push_log(LogLevel level, std::format_string<Args...> fmt,
                Args&&... args) {
    if (m_queue.try_emplace(level, fmt.get(), std::forward<Args>(args)...)) {
      m_signal.fetch_add(1, std::memory_order_release);
      m_signal.notify_one();
    }
    // Silently drop if queue is full (non-blocking guarantee)
  }

  void shutdown();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  ~Logger();

 private:
  void consumer_thread_loop();

  std::atomic<bool> m_done{false};
  alignas(64) std::atomic<uint64_t> m_signal{0};
  MPSCQueue<internal::MessagePayload, 1024> m_queue;
  std::jthread m_consumer_thread;
  std::vector<std::unique_ptr<Sink>> m_sinks;
};

// Global/default logger functions (optional but convenient)
// This provides an easy migration path from the old singleton API.
void init_default_logger(std::vector<std::unique_ptr<Sink>> sinks);
Logger* default_logger();

template <LogLevel level, typename... Args>
inline void log(std::format_string<Args...> fmt, Args&&... args) {
  if constexpr (level >= LOG_ACTIVE_LEVEL) {
    if (auto* logger = default_logger(); logger) {
      logger->push_log(level, fmt, std::forward<Args>(args)...);
    }
  }
}

template <typename... Args>
inline void log_debug(std::format_string<Args...> fmt, Args&&... args) {
  log<LOG_LEVEL_DEBUG>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_info(std::format_string<Args...> fmt, Args&&... args) {
  log<LOG_LEVEL_INFO>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_warn(std::format_string<Args...> fmt, Args&&... args) {
  log<LOG_LEVEL_WARN>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void log_error(std::format_string<Args...> fmt, Args&&... args) {
  log<LOG_LEVEL_ERROR>(fmt, std::forward<Args>(args)...);
}

}  // namespace log_library
