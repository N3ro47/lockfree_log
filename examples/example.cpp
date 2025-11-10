#include <log_library/logger.h>
#include <log_library/sinks/file_sink.h>

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

void worker_thread(int id) {
  log_library::log_info("Worker thread {} starting.", id);
  for (int i = 0; i < 5; ++i) {
    log_library::log_debug("Worker {} logging message #{}", id, i);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  log_library::log_warn("Worker thread {} finished.", id);
}

int main() {
  // Initialize the default logger with a file sink
  std::vector<std::unique_ptr<log_library::Sink>> sinks;
  sinks.push_back(log_library::create_file_sink());
  log_library::init_default_logger(std::move(sinks));

  log_library::log_info("Main thread started. Spawning workers.");

  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back(worker_thread, i + 1);
  }

  for (auto& t : workers) {
    t.join();
  }

  log_library::log_error("All workers finished. Main thread shutting down.");

  if (auto* logger = log_library::default_logger(); logger) {
    logger->shutdown();
  }

  return 0;
}
