#pragma once

#include "config.h"
#include <format>
#include <string_view>
#include <memory>
#include <atomic>
#include <thread>
#include "internal/message_payload.hpp"
#include "mpsc_queue.hpp"

namespace log_library {

// Forward declarations
class Sink;

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
    void consumer_thread_loop();

    std::atomic<bool> m_done{false};
    alignas(64) std::atomic<uint64_t> m_signal{0};
    MPSCQueue<internal::MessagePayload, 1024> m_queue;
    std::jthread m_consumer_thread;
    std::unique_ptr<Sink> m_sink;
};

} // namespace log_library


#define LOG(level, fmt, ...)                                                   \
  do {                                                                         \
    if constexpr (level >= LOG_ACTIVE_LEVEL) {                                 \
      log_library::Logger::instance().push_log(                                \
          log_library::internal::MessagePayload(level,                         \
                                           fmt __VA_OPT__(, ) __VA_ARGS__));   \
    }                                                                          \
  } while (false)


#define LOG_DEBUG(fmt, ...) LOG(LOG_LEVEL_DEBUG, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG(LOG_LEVEL_INFO, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG(LOG_LEVEL_WARN, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG_LEVEL_ERROR, fmt __VA_OPT__(, ) __VA_ARGS__)