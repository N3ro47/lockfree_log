#include <log_library/logger.h>
#include <log_library/sink.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// NullSink remains the same
class NullSink : public log_library::Sink {
  public:
    void write(const std::string& message, LogLevel level) override {}
    void flush() override {}
};

// producer_latency_task remains the same
void producer_latency_task(size_t messages_to_send, bool retry_on_fail,
                           log_library::Logger& logger,
                           std::vector<long long>& latencies) {
    latencies.reserve(messages_to_send);
    for (size_t i = 0; i < messages_to_send; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        if (retry_on_fail) {
            while (!logger.push_log(LOG_LEVEL_INFO, "Guaranteed message {}", i)) {
                std::this_thread::yield();
            }
        } else {
            logger.push_log(LOG_LEVEL_INFO, "Non-blocking message {}", i);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count();
        latencies.push_back(duration_ns);
    }
}

// print_stats remains the same
void print_stats(size_t thread_count, std::vector<long long>& all_latencies) {
    if (all_latencies.empty()) {
        std::cout << thread_count << ",0,0,0,0,0,0,0,0" << std::endl;
        return;
    }
    std::sort(all_latencies.begin(), all_latencies.end());
    long long min_val = all_latencies.front();
    long long max_val = all_latencies.back();
    long double sum = std::accumulate(all_latencies.begin(), all_latencies.end(), 0.0L);
    long double avg_val = sum / all_latencies.size();
    auto get_percentile = [&](double p) {
        size_t index = static_cast<size_t>(all_latencies.size() * p);
        if (index >= all_latencies.size()) index = all_latencies.size() - 1;
        return all_latencies[index];
    };
    std::cout << thread_count << "," << all_latencies.size() << "," << min_val << ","
              << max_val << "," << static_cast<long long>(avg_val) << ","
              << get_percentile(0.50) << "," << get_percentile(0.99) << ","
              << get_percentile(0.999) << "," << get_percentile(0.9999) << std::endl;
}

// VVVV --- NEW FUNCTION TO DUMP RAW DATA --- VVVV
void print_raw_data(const std::vector<long long>& all_latencies) {
    // Print a header for the CSV file
    std::cout << "latency_ns\n";
    for (long long latency : all_latencies) {
        std::cout << latency << "\n";
    }
}


int main(int argc, char* argv[]) {
    // VVVV --- NEW: Command-line argument parsing --- VVVV
    bool raw_output = false;
    if (argc > 1 && std::string(argv[1]) == "--raw") {
        raw_output = true;
    }

    const size_t messages_per_thread = 100'000;
    const std::vector<size_t> num_threads_per_run = {1, 2, 4, 8};

    if (raw_output) {
        // --- RAW OUTPUT MODE ---
        // For raw output, we typically only care about one scenario at a time.
        // Let's run the most interesting one: 8-thread, guaranteed latency.
        const size_t thread_count = 8;
        
        std::vector<std::unique_ptr<log_library::Sink>> sinks;
        sinks.push_back(std::make_unique<NullSink>());
        log_library::Logger logger(std::move(sinks));

        std::vector<std::thread> threads;
        std::vector<std::vector<long long>> thread_latencies(thread_count);
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back(producer_latency_task, messages_per_thread, true, std::ref(logger), std::ref(thread_latencies[i]));
        }
        for (auto& t : threads) t.join();

        std::vector<long long> all_latencies;
        all_latencies.reserve(thread_count * messages_per_thread);
        for (const auto& v : thread_latencies) {
            all_latencies.insert(all_latencies.end(), v.begin(), v.end());
        }
        print_raw_data(all_latencies);

    } else {
        // --- SUMMARY STATISTICS MODE (the old behavior) ---
        std::cout << std::fixed << std::setprecision(0);

        std::cout << "\n--- SCENARIO: Non-Blocking (Fire and Forget) Latency ---\n" << std::endl;
        std::cout << "Threads,Samples,Min(ns),Max(ns),Avg(ns),p50(ns),p99(ns),p99.9(ns),p99.99(ns)" << std::endl;
        for (size_t thread_count : num_threads_per_run) {
            std::vector<std::unique_ptr<log_library::Sink>> sinks;
            sinks.push_back(std::make_unique<NullSink>());
            log_library::Logger logger(std::move(sinks));
            std::vector<std::thread> threads;
            std::vector<std::vector<long long>> thread_latencies(thread_count);
            for (size_t i = 0; i < thread_count; ++i) {
                threads.emplace_back(producer_latency_task, messages_per_thread, false, std::ref(logger), std::ref(thread_latencies[i]));
            }
            for (auto& t : threads) t.join();
            std::vector<long long> all_latencies;
            for (const auto& v : thread_latencies) {
                all_latencies.insert(all_latencies.end(), v.begin(), v.end());
            }
            print_stats(thread_count, all_latencies);
        }

        std::cout << "\n--- SCENARIO: Guaranteed (Retry on Fail) Latency ---\n" << std::endl;
        std::cout << "Threads,Samples,Min(ns),Max(ns),Avg(ns),p50(ns),p99(ns),p99.9(ns),p99.99(ns)" << std::endl;
        for (size_t thread_count : num_threads_per_run) {
            std::vector<std::unique_ptr<log_library::Sink>> sinks;
            sinks.push_back(std::make_unique<NullSink>());
            log_library::Logger logger(std::move(sinks));
            std::vector<std::thread> threads;
            std::vector<std::vector<long long>> thread_latencies(thread_count);
            for (size_t i = 0; i < thread_count; ++i) {
                threads.emplace_back(producer_latency_task, messages_per_thread, true, std::ref(logger), std::ref(thread_latencies[i]));
            }
            for (auto& t : threads) t.join();
            std::vector<long long> all_latencies;
            for (const auto& v : thread_latencies) {
                all_latencies.insert(all_latencies.end(), v.begin(), v.end());
            }
            print_stats(thread_count, all_latencies);
        }
    }

    return 0;
}