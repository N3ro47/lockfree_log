#include <log_library/file_sink.h>
#include <log_library/file_sink_config.h>
#include <log_library/logger.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<bool> keep_running = true;

void signal_handler(int signum) {
  std::cout << "Signal (" << signum << ") received, shutting down."
            << std::endl;
  keep_running = false;
}

void worker_thread(int id) {
  int message_count = 0;
  while (keep_running) {
    log_library::log_info("Worker {} logging message #{}", id, ++message_count);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  log_library::FileSinkConfig config;
  config.log_directory = "./sanitizer_logs/";
  config.base_filename = "sanitizer_test";
  config.max_file_size = 10 * 1024;   // 10 KB
  config.system_max_use = 50 * 1024;  // 50 KB total

  log_library::Logger::init(log_library::create_file_sink(config));

  log_library::log_info("Sanitizer test started. Running for 15 seconds.");

  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back(worker_thread, i + 1);
  }

  auto start_time = std::chrono::steady_clock::now();
  while (keep_running) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    if (elapsed.count() >= 15) {
      log_library::log_warn(
          "Test duration reached, shutting down automatically.");
      keep_running = false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  for (auto& t : workers) {
    t.join();
  }

  log_library::log_error("All workers finished. Main thread shutting down.");

  log_library::Logger::instance().shutdown();

  std::cout << "Sanitizer test finished cleanly." << std::endl;

  return 0;
}
