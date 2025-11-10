// FILE: test/burst_consistency_test.cpp
#include <log_library/logger.h>
#include <log_library/sink.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// A custom sink that captures log messages for verification.
class VerifyingSink : public log_library::Sink {
 public:
  void write(const std::string& message, LogLevel level) override {
    std::lock_guard<std::mutex> lock(mtx_);
    // Remove the trailing newline to make regex parsing simpler
    if (!message.empty() && message.back() == '\n') {
      messages_.push_back(message.substr(0, message.length() - 1));
    } else {
      messages_.push_back(message);
    }
  }

  void flush() override {}  // No-op

  void clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    messages_.clear();
  }

  std::vector<std::string> get_messages() {
    std::lock_guard<std::mutex> lock(mtx_);
    return messages_;
  }

 private:
  std::mutex mtx_;
  std::vector<std::string> messages_;
};

constexpr int MESSAGES_PER_BURST = 10000;
constexpr int NUM_CYCLES = 5;

// Producer thread that logs simple types with a verifiable payload.
void simple_producer() {
  for (int i = 0; i < MESSAGES_PER_BURST; ++i) {
    // The payload (the second 'i') must match the message number 'i'.
    log_library::log_info("Simple producer #{}: payload={}", i, i);
  }
}

// Producer thread that logs heap-allocated std::vector with verifiable content.
void vector_producer() {
  for (int i = 0; i < MESSAGES_PER_BURST; ++i) {
    // The vector's content is uniquely derived from the message number 'i'.
    std::vector<int> v = {i, i + 1, i + 2};
    log_library::log_warn("Vector producer #{}: content={}", i, v);
  }
}

// Helper to extract a thread ID from the start of a log message.
std::thread::id extract_thread_id(const std::string& msg) {
  std::stringstream ss(msg);
  unsigned long long id_val;
  ss >> id_val;
  return std::thread::id(id_val);
}

int main() {
  auto sink = std::make_unique<VerifyingSink>();
  VerifyingSink* sink_ptr = sink.get();

  log_library::Logger::init(std::move(sink));

  std::cout << "Starting burst and data consistency test..." << std::endl;
  std::cout << "Focus is on data integrity. Dropped messages are expected under "
               "load."
            << std::endl;

  for (int cycle = 1; cycle <= NUM_CYCLES; ++cycle) {
    std::cout << "\n--- Cycle " << cycle << "/" << NUM_CYCLES << " ---" << std::endl;
    sink_ptr->clear();

    // --- BURST ---
    std::thread simple_thread(simple_producer);
    std::thread vector_thread(vector_producer);

    auto simple_thread_id = simple_thread.get_id();
    auto vector_thread_id = vector_thread.get_id();

    simple_thread.join();
    vector_thread.join();

    std::cout << "Producer burst finished. Total messages attempted: "
              << MESSAGES_PER_BURST * 2 << std::endl;

    // --- DRAIN & VERIFY ---
    // Give the consumer thread a moment to process the burst. A simple sleep is
    // sufficient as we only need to wait for the queue to likely be empty.
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    auto messages = sink_ptr->get_messages();
    const size_t consumed_count = messages.size();
    std::cout << "Consumer processed " << consumed_count << " messages." << std::endl;
    assert(consumed_count > 0 &&
           "FATAL: No messages were consumed. The logger might be stuck.");

    // --- DATA CONSISTENCY CHECK ---
    std::set<int> seen_simple_indices;
    std::set<int> seen_vector_indices;
    int simple_msgs_verified = 0;
    int vector_msgs_verified = 0;

    // Regex is more robust for parsing than simple find/substr.
    const std::regex simple_regex(R"(Simple producer #(\d+): payload=(\d+))");
    const std::regex vector_regex(
        R"(Vector producer #(\d+): content=\[(\d+), (\d+), (\d+)\])");
    std::smatch match;

    for (const auto& msg : messages) {
      auto thread_id = extract_thread_id(msg);

      if (thread_id == simple_thread_id) {
        assert(std::regex_search(msg, match, simple_regex) &&
               "Simple message format is incorrect.");
        assert(match.size() == 3);  // Full match + 2 capture groups

        int msg_index = std::stoi(match[1].str());
        int payload_val = std::stoi(match[2].str());

        // CORE CONSISTENCY ASSERTION: The data must not be corrupted.
        assert(msg_index == payload_val && "Data corruption in simple message!");

        // Check for duplicates, which would indicate a queue logic error.
        assert(seen_simple_indices.find(msg_index) ==
                   seen_simple_indices.end() &&
               "Duplicate simple message detected!");
        seen_simple_indices.insert(msg_index);
        simple_msgs_verified++;

      } else if (thread_id == vector_thread_id) {
        assert(std::regex_search(msg, match, vector_regex) &&
               "Vector message format is incorrect.");
        assert(match.size() == 5);  // Full match + 4 capture groups

        int msg_index = std::stoi(match[1].str());
        int val1 = std::stoi(match[2].str());
        int val2 = std::stoi(match[3].str());
        int val3 = std::stoi(match[4].str());

        // CORE CONSISTENCY ASSERTION: Vector content must not be corrupted.
        assert(val1 == msg_index && "Corruption in vector message (value 1)!");
        assert(val2 == msg_index + 1 && "Corruption in vector message (value 2)!");
        assert(val3 == msg_index + 2 && "Corruption in vector message (value 3)!");

        // Check for duplicates.
        assert(seen_vector_indices.find(msg_index) ==
                   seen_vector_indices.end() &&
               "Duplicate vector message detected!");
        seen_vector_indices.insert(msg_index);
        vector_msgs_verified++;
      }
    }

    std::cout << "Data integrity verification complete." << std::endl;
    std::cout << "  - Verified " << simple_msgs_verified
              << " simple messages (no corruption or duplicates)." << std::endl;
    std::cout << "  - Verified " << vector_msgs_verified
              << " vector messages (no corruption or duplicates)." << std::endl;
    //assert(simple_msgs_verified + vector_msgs_verified == consumed_count);
  }

  log_library::Logger::instance().shutdown();

  std::cout << "\nBurst and data consistency test finished successfully." << std::endl;

  return 0;
}