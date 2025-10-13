#include <atomic>
#include <log_library/logger.h>
#include "internal/sink.h"
#include "internal/file_sink_config.h"
#include "internal/sink_config.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>

// Platform-specific includes
#ifdef _WIN32
#include "windows_file_sink.cpp"
#else
#include "linux_file_sink.cpp"
#endif

namespace log_library {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_sink(std::make_unique<LOG_SINK_TYPE>()) {
    m_consumer_thread = std::jthread(&Logger::consumer_thread_loop, this);
}

Logger::~Logger() {
    if (!m_done.load(std::memory_order_acquire)) {
        shutdown();
    }
}

void Logger::push_log(internal::MessagePayload&& payload) {
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
            std::format_to(std::back_inserter(buffer), "{}: ", to_string(payload.level));
            payload.formatter(buffer, payload.format_string, payload.arg_buffer);
            buffer.push_back('\n');
            m_sink->write(buffer, payload.level);
        } else {
            auto current = m_signal.load(std::memory_order_acquire);
            m_signal.wait(current, std::memory_order_acquire);
        }
    }

    //drain loop
    while (m_queue.try_pop(payload)) {
        buffer.clear();
        std::format_to(std::back_inserter(buffer), "{}: ", to_string(payload.level));
        payload.formatter(buffer, payload.format_string, payload.arg_buffer);
        buffer.push_back('\n');
        m_sink->write(buffer, payload.level);
    }
    
    m_sink->flush();
}

} // namespace log_library