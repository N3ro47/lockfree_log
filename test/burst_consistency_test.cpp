#include <log_library/logger.h>
#include <log_library/sink.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class VerifyingSink : public log_library::Sink {
 public:
  void write(const std::string& message, LogLevel level) override {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!message.empty() && message.back() == '\n') {
      messages_.push_back(message.substr(0, message.length() - 1));
    } else {
      messages_.push_back(message);
    }
  }

  void flush() override {}

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

struct TestPayload {
  int msg_index;
  int val1;
  int val2;
};

template <>
struct std::formatter<TestPayload> : std::formatter<std::string> {
  auto format(const TestPayload& p, std::format_context& ctx) const {
    return std::format_to(ctx.out(), "[{}, {}, {}]", p.msg_index, p.val1,
                          p.val2);
  }
};

void simple_producer() {
  for (int i = 0; i < MESSAGES_PER_BURST; ++i) {
    log_library::log_info("Simple producer #{}: payload={}", i, i);
  }
}

void struct_producer() {
  for (int i = 0; i < MESSAGES_PER_BURST; ++i) {
    TestPayload p = {i, i + 1, i + 2};
    log_library::log_warn("Struct producer #{}: content={}", i, p);
  }
}

std::thread::id extract_thread_id(const std::string& msg) {
  std::stringstream ss(msg);
  unsigned long long id_val;
  ss >> id_val;
  return std::thread::id(id_val);
}

int main() {
  auto sink = std::make_unique<VerifyingSink>();
  VerifyingSink* sink_ptr = sink.get();

  std::vector<std::unique_ptr<log_library::Sink>> sinks;
  sinks.push_back(std::move(sink));
  log_library::init_default_logger(std::move(sinks));

  std::cout << "Starting burst and data consistency test..." << std::endl;

  for (int cycle = 1; cycle <= NUM_CYCLES; ++cycle) {
    std::cout << "\n--- Cycle " << cycle << "/" << NUM_CYCLES << " ---"
              << std::endl;
    sink_ptr->clear();

    std::thread simple_thread(simple_producer);
    std::thread struct_thread(struct_producer);

    auto simple_thread_id = simple_thread.get_id();
    auto struct_thread_id = struct_thread.get_id();

    simple_thread.join();
    struct_thread.join();

    std::cout << "Producer burst finished. Total messages attempted: "
              << MESSAGES_PER_BURST * 2 << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    auto messages = sink_ptr->get_messages();
    const size_t consumed_count = messages.size();
    std::cout << "Consumer processed " << consumed_count << " messages."
              << std::endl;
    assert(consumed_count > 0 &&
           "FATAL: No messages were consumed. The logger might be stuck.");

    std::set<int> seen_simple_indices;
    std::set<int> seen_struct_indices;
    int simple_msgs_verified = 0;
    int struct_msgs_verified = 0;

    const std::regex simple_regex(R"(Simple producer #(\d+): payload=(\d+))");
    const std::regex struct_regex(
        R"(Struct producer #(\d+): content=\[(\d+), (\d+), (\d+)\])");
    std::smatch match;

    for (const auto& msg : messages) {
      auto thread_id = extract_thread_id(msg);

      if (thread_id == simple_thread_id) {
        assert(std::regex_search(msg, match, simple_regex) &&
               "Simple message format is incorrect.");
        assert(match.size() == 3);

        int msg_index = std::stoi(match[1].str());
        int payload_val = std::stoi(match[2].str());

        assert(msg_index == payload_val &&
               "Data corruption in simple message!");

        assert(seen_simple_indices.find(msg_index) ==
                   seen_simple_indices.end() &&
               "Duplicate simple message detected!");
        seen_simple_indices.insert(msg_index);
        simple_msgs_verified++;

      } else if (thread_id == struct_thread_id) {
        assert(std::regex_search(msg, match, struct_regex) &&
               "Struct message format is incorrect.");
        assert(match.size() == 5);

        int msg_index = std::stoi(match[1].str());
        int val1 = std::stoi(match[2].str());
        int val2 = std::stoi(match[3].str());
        int val3 = std::stoi(match[4].str());

        assert(val1 == msg_index && "Corruption in struct message (value 1)!");
        assert(val2 == msg_index + 1 &&
               "Corruption in struct message (value 2)!");
        assert(val3 == msg_index + 2 &&
               "Corruption in struct message (value 3)!");

        assert(seen_struct_indices.find(msg_index) ==
                   seen_struct_indices.end() &&
               "Duplicate struct message detected!");
        seen_struct_indices.insert(msg_index);
        struct_msgs_verified++;
      }
    }

    std::cout << "Data integrity verification complete." << std::endl;
    std::cout << "  - Verified " << simple_msgs_verified
              << " simple messages (no corruption or duplicates)." << std::endl;
    std::cout << "  - Verified " << struct_msgs_verified
              << " struct messages (no corruption or duplicates)." << std::endl;
  }

  if (auto* logger = log_library::default_logger(); logger) {
    logger->shutdown();
  }

  std::cout << "\nBurst and data consistency test finished successfully."
            << std::endl;

  return 0;
}