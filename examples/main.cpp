#include <log_library/logger.h>

#include <chrono>
#include <thread>
#include <vector>

void worker_thread(int id) {
  LOG_INFO("Worker thread {} starting.", id);
  for (int i = 0; i < 5; ++i) {
    LOG_DEBUG("Worker {} logging message #{}", id, i);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  LOG_WARN("Worker thread {} finished.", id);
}

int main() {
  LOG_INFO("Main thread started. Spawning workers.");

  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) {
    workers.emplace_back(worker_thread, i + 1);
  }

  for (auto &t : workers) {
    t.join();
  }

  LOG_ERROR("All workers finished. Main thread shutting down.");

  log_library::Logger::instance().shutdown();

  return 0;
}
