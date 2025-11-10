#pragma once

#include <log_library/config.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

namespace log_library::internal {

constexpr size_t MAX_ARG_BUFFER_SIZE = 24;

struct MessagePayload {
  using FormatterFunc = void (*)(std::string&, std::string_view,
                                 const std::byte*);

  std::string_view format_string;
  std::thread::id thread_id;
  FormatterFunc formatter;
  LogLevel level;
  alignas(std::max_align_t) std::byte arg_buffer[MAX_ARG_BUFFER_SIZE];

  MessagePayload() = default;
  ~MessagePayload() = default;
  MessagePayload(const MessagePayload& other) = default;
  MessagePayload& operator=(const MessagePayload& other) = default;

  template <typename... Args>
  MessagePayload(LogLevel lvl, std::string_view fmt, Args&&... args)
      : format_string(fmt),
        thread_id(std::this_thread::get_id()),
        formatter(&format_message<std::decay_t<Args>...>),
        level(lvl) {
    using TupleType = std::tuple<std::decay_t<Args>...>;

    // CORE CHANGE: Enforce that the arguments are trivially copyable.
    static_assert((std::is_trivially_copyable_v<std::decay_t<Args>> && ...),
                  "All log arguments must be trivially copyable.");

    static_assert(sizeof(TupleType) <= MAX_ARG_BUFFER_SIZE,
                  "Log arguments exceed maximum payload size.");

    std::construct_at(reinterpret_cast<TupleType*>(arg_buffer),
                      std::forward<Args>(args)...);
  }

 private:
  template <typename... DecayedArgs>
  static void format_message(std::string& out, std::string_view fmt,
                             const std::byte* buffer) {
    const auto* arg_tuple = std::launder(
        reinterpret_cast<const std::tuple<DecayedArgs...>*>(buffer));

    std::apply(
        [&](const DecayedArgs&... args) {
          std::vformat_to(std::back_inserter(out), fmt,
                          std::make_format_args(args...));
        },
        *arg_tuple);
  }
};

}  // namespace log_library::internal
