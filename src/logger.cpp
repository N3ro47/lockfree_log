#include <log_library/logger.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <log_library/file_sink_config.h>
#include <log_library/sink.h>
#include "internal/sink_config.h"

// Platform-specific includes for the *default* sink
#ifdef _WIN32
#include "windows_file_sink.cpp"
#else
#include "linux_file_sink.cpp"
#endif

namespace {

struct LoggerState {
  std::unique_ptr<log_library::Sink> sink = nullptr;
  std::mutex mutex;
};

LoggerState& get_logger_state() {
  static LoggerState state;
  return state;
}
}  // namespace

namespace log_library {

void Logger::init(std::unique_ptr<Sink> sink) {
  auto& state = get_logger_state();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.sink = std::move(sink);
}

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

Logger::Logger() {
  auto& state = get_logger_state();
  std::lock_guard<std::mutex> lock(state.mutex);

  if (state.sink) {
    m_sink = std::move(state.sink);
  } else {
    // Default behavior if Logger::init() was not called.
    m_sink = std::make_unique<LOG_SINK_TYPE>();
  }

  m_consumer_thread = std::jthread(&Logger::consumer_thread_loop, this);
}

Logger::~Logger() {
  if (!m_done.load(std::memory_order_acquire)) {
    shutdown();
  }
}

void Logger::push_log(internal::MessagePayload&& payload) {
  // Use try_emplace to construct the payload in-place in the queue's buffer.
  if (m_queue.try_emplace(std::move(payload))) {
    m_signal.fetch_add(1, std::memory_order_release);
    m_signal.notify_one();
  }
  // Silently drop if queue is full (non-blocking guarantee)
}

void Logger::shutdown() {
  m_done.store(true, std::memory_order_release);
  m_signal.fetch_add(1, std::memory_order_release);
  m_signal.notify_one();

  if (m_consumer_thread.joinable()) {
    m_consumer_thread.join();
  }
}

void Logger::consumer_thread_loop() {
  std::string buffer;
  using Payload = internal::MessagePayload;
  Payload payload;

  while (!m_done.load(std::memory_order_acquire)) {
    if (m_queue.try_pop(payload)) {
      buffer.clear();
      std::format_to(std::back_inserter(buffer), "{}: ",
                     to_string(payload.level));
      payload.formatter(buffer, payload.format_string, payload.arg_buffer);
      buffer.push_back('\n');
      m_sink->write(buffer, payload.level);
    } else {
      auto current = m_signal.load(std::memory_order_acquire);
      m_signal.wait(current, std::memory_order_acquire);
    }
  }

  // drain loop
  while (m_queue.try_pop(payload)) {
    buffer.clear();
    std::format_to(std::back_inserter(buffer), "{}: ",
                   to_string(payload.level));
    payload.formatter(buffer, payload.format_string, payload.arg_buffer);
    buffer.push_back('\n');
    m_sink->write(buffer, payload.level);
  }

  m_sink->flush();
}

}  // namespace log_library