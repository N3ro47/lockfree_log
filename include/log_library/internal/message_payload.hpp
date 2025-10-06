#pragma once

#include <cstddef>
#include <iterator>
#include <log_library/config.h>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

namespace log_library::internal {

constexpr size_t MAX_ARG_BUFFER_SIZE = 128;

struct MessagePayload {
  LogLevel level;
  std::thread::id thread_id;
  std::string_view format_string;

  alignas(std::max_align_t) std::byte arg_buffer[MAX_ARG_BUFFER_SIZE];
  using FormatterFunc = void (*)(std::string &, std::string_view,
                                 const std::byte *);
  FormatterFunc formatter;

  MessagePayload() = default;

  template <typename... Args>
  MessagePayload(LogLevel lvl, std::string_view fmt, Args &&...args)
      : level(lvl), thread_id(std::this_thread::get_id()), format_string(fmt) {
    static_assert(sizeof(std::tuple<Args...>) <= MAX_ARG_BUFFER_SIZE,
                  "Log arguments exceed maximum payload size.");
    new (arg_buffer) std::tuple<Args...>(std::forward<Args>(args)...);
    formatter = &format_message<Args...>;
  }

private:
  template <typename... Args>
  static void format_message(std::string &out, std::string_view fmt,
                             const std::byte *buffer) {
    const auto *arg_tuple =
        reinterpret_cast<const std::tuple<Args...> *>(buffer);

    std::apply(
        [&](const Args &...args) {
          std::vformat_to(std::back_inserter(out), fmt,
                          std::make_format_args(args...));
        },
        *arg_tuple);
  }
};

} // namespace log_library::internal
