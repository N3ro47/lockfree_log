#include <atomic>
#include <log_library/logger.h>
#include <log_library/sink.h>
#include <log_library/file_sink_config.h>
#include <log_library/sink_config.h>
#include "mpsc_queue.hpp"
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

class Logger::Impl {
public:
    ~Impl() = default;
    std::atomic<bool> m_done{false};

    void consumer_thread_loop();

    using Payload = internal::MessagePayload;
    MPSCQueue<Payload, 1024> m_queue;
    std::jthread m_consumer_thread;
    std::unique_ptr<Sink> m_sink;
    
    Impl() {
        m_sink = std::make_unique<LOG_SINK_TYPE>();
    }
};

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : pimpl(std::make_unique<Impl>()) {
    pimpl->m_consumer_thread = std::jthread(&Logger::Impl::consumer_thread_loop, pimpl.get());
}

Logger::~Logger() {
    if (!pimpl->m_done.load(std::memory_order_acquire)) {
        shutdown();
    }
};

void Logger::push_log(internal::MessagePayload&& payload) {
    pimpl->m_queue.try_emplace(std::move(payload));
    pimpl->m_done.notify_one();
}

void Logger::shutdown() {
    pimpl->m_done.store(true, std::memory_order_release);

    if (pimpl->m_consumer_thread.joinable()) {
        pimpl->m_consumer_thread.join();
    }
}

void Logger::Impl::consumer_thread_loop() {
    std::string buffer;
    Payload payload;

    while (!m_done.load(std::memory_order_acquire)) {
        if (m_queue.try_pop(payload)) {
            buffer.clear();
            buffer += to_string(payload.level);
            buffer += ": ";
            payload.formatter(buffer, payload.format_string, payload.arg_buffer);
            buffer += "\n";
            m_sink->write(buffer, payload.level);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    //drain loop
    while (m_queue.try_pop(payload)) {
        buffer.clear();
        buffer += to_string(payload.level);
        buffer += ": ";
        payload.formatter(buffer, payload.format_string, payload.arg_buffer);
        buffer += "\n";
        m_sink->write(buffer, payload.level);
    }
    
    m_sink->flush();
}

} // namespace log_library