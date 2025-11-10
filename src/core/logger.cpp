#include <log_library/logger.h>
#include <log_library/sink.h>

#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

namespace {
std::unique_ptr<log_library::Logger> g_default_logger = nullptr;
std::mutex g_default_logger_mutex;
}  // namespace

namespace log_library {

void init_default_logger(std::vector<std::unique_ptr<Sink>> sinks) {
  std::lock_guard<std::mutex> lock(g_default_logger_mutex);
  if (!g_default_logger) {
    g_default_logger = std::make_unique<Logger>(std::move(sinks));
  }
}

Logger* default_logger() {
  return g_default_logger.get();
}

Logger::Logger(std::vector<std::unique_ptr<Sink>> sinks)
    : m_sinks(std::move(sinks)) {
  m_consumer_thread = std::jthread(&Logger::consumer_thread_loop, this);
}

Logger::~Logger() {
  if (!m_done.load(std::memory_order_acquire)) {
    shutdown();
  }
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
  internal::MessagePayload payload;

  while (!m_done.load(std::memory_order_acquire)) {
    if (m_queue.try_pop(payload)) {
      buffer.clear();
      std::format_to(std::back_inserter(buffer), "{}: ",
                     to_string(payload.level));
      payload.formatter(buffer, payload.format_string, payload.arg_buffer);
      buffer.push_back('\n');

      // Dispatch to all sinks
      for (const auto& sink : m_sinks) {
        sink->write(buffer, payload.level);
      }
    } else {
      auto current = m_signal.load(std::memory_order_acquire);
      m_signal.wait(current, std::memory_order_acquire);
    }
  }

  // Drain the queue after shutdown signal
  while (m_queue.try_pop(payload)) {
    buffer.clear();
    std::format_to(std::back_inserter(buffer), "{}: ",
                   to_string(payload.level));
    payload.formatter(buffer, payload.format_string, payload.arg_buffer);
    buffer.push_back('\n');
    for (const auto& sink : m_sinks) {
      sink->write(buffer, payload.level);
    }
  }

  // Flush all sinks
  for (const auto& sink : m_sinks) {
    sink->flush();
  }
}

}  // namespace log_library