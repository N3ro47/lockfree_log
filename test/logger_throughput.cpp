#include <log_library/logger.h>
#include <log_library/sink.h>

#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <memory>
#include <immintrin.h>

class NullSink : public log_library::Sink {
  public:
    void write(const std::string& message, LogLevel level) override {
      message_count_.fetch_add(1, std::memory_order_relaxed);
    }
    void flush() override {}
    void reset_count() {
      message_count_.store(0, std::memory_order_relaxed);
    }
    size_t get_message_count() const {
      return message_count_.load(std::memory_order_relaxed);
    }
  private:
    std::atomic<size_t> message_count_{0};
};

int main() {
  auto null_sink = std::make_unique<NullSink>();
  NullSink* sink_ptr = null_sink.get();

  std::vector<std::unique_ptr<log_library::Sink>> sinks;
  sinks.push_back(std::move(null_sink));
  log_library::init_default_logger(std::move(sinks));

  std::vector<size_t> num_threads_per_run{1, 2, 4, 8};

  std::cout << "--- Measuring Sustainable End-to-End Throughput ---" << std::endl;

  for (std::size_t thread_count : num_threads_per_run) {
    const size_t messages_per_thread = 1'000'000;
    const size_t total_messages = messages_per_thread * thread_count;
    
    std::vector<std::thread> threads;
    sink_ptr->reset_count();

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([=]() {
            size_t enqueued_count = 0;
            while (enqueued_count < messages_per_thread) {
                if (log_library::log_info("Benchmark message from thread {} msg {}", i, enqueued_count)) {
                    enqueued_count++;
                } else {
                    _mm_pause();
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    while (sink_ptr->get_message_count() < total_messages) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    
    double throughput = static_cast<double>(total_messages) / duration.count();
    size_t actual_messages = sink_ptr->get_message_count();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Threads: " << thread_count
              << ", Total Msgs: " << total_messages
              << ", Time: " << duration.count() << " sec"
              << ", Throughput: " << throughput / 1'000'000.0 << " M msgs/sec"
              << ", Received: " << actual_messages << std::endl;
  }

  if (auto* logger = log_library::default_logger(); logger) {
      logger->shutdown();
  }

  return 0;
}