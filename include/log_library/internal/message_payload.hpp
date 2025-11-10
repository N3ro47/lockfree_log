#pragma once

#include <log_library/config.h>

#include <cstddef>
#include <cstring>
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

constexpr size_t MAX_ARG_BUFFER_SIZE = 72;

struct MessagePayload {
  using FormatterFunc = void (*)(std::string&, std::string_view,
                                 const std::byte*);
  using DestructorFunc = void (*)(std::byte*);
  using MoverFunc = void (*)(std::byte*, std::byte*);

  std::string_view format_string;  // 16
  std::thread::id thread_id;       // 8
  FormatterFunc formatter;         // 8
  DestructorFunc destructor;       // 8
  MoverFunc mover;                 // 8  
  LogLevel level;                  // 4
  // 4-byte padding inserted by compiler to meet alignment requirements,
  // bringing metadata size to 48 bytes. MAX_ARG_BUFFER_SIZE bytes in-place
  // buffer for arguments, making the total payload size a 128 bytes.
  alignas(std::max_align_t) std::byte arg_buffer[MAX_ARG_BUFFER_SIZE];

  MessagePayload() : formatter(nullptr), destructor(nullptr) {}
  ~MessagePayload() {
    if (destructor) {
      destructor(arg_buffer);
    }
  }

  MessagePayload(const MessagePayload&) = delete;
  MessagePayload& operator=(const MessagePayload&) = delete;

  MessagePayload(MessagePayload&& other) noexcept
      : format_string(other.format_string),
        thread_id(other.thread_id),
        formatter(other.formatter),
        destructor(other.destructor),
        mover(other.mover),
        level(other.level) {
        if (mover) {
          mover(arg_buffer, other.arg_buffer);
        }
    other.destructor = nullptr;
  }

  MessagePayload& operator=(MessagePayload&& other) noexcept {
    if (this != &other) {
      if (destructor) {
        destructor(arg_buffer);
      }

      format_string = other.format_string;
      thread_id = other.thread_id;
      formatter = other.formatter;
      destructor = other.destructor;
      mover = other.mover;
      level = other.level; 
      if (mover) {
        mover(arg_buffer, other.arg_buffer);
      }
      other.destructor = nullptr;
    }
    return *this;
  }

  template <typename... Args>
  MessagePayload(LogLevel lvl, std::string_view fmt, Args&&... args)
      : level(lvl),
        thread_id(std::this_thread::get_id()),
        format_string(fmt),
        formatter(&format_message<std::decay_t<Args>...>),
        destructor(&destoy_tuple<std::decay_t<Args>...>),
        mover(&move_tuple<std::decay_t<Args>...>) {
    
    using TupleType = std::tuple<std::decay_t<Args>...>;

    static_assert(sizeof(TupleType) <= MAX_ARG_BUFFER_SIZE,
                  "Log arguments exceed maximum payload size.");

    std::construct_at(reinterpret_cast<TupleType*>(arg_buffer),
                      std::forward<Args>(args)...);
  }

 private:
  template <typename... DecayedArgs>
  static void format_message(std::string& out, std::string_view fmt,
                             const std::byte* buffer) {
    const auto* arg_tuple =
        std::launder(reinterpret_cast<const std::tuple<DecayedArgs...>*>(buffer));

    std::apply(
        [&](const DecayedArgs&... args) {
          std::vformat_to(std::back_inserter(out), fmt,
                          std::make_format_args(args...));
        },
        *arg_tuple);
  }

  template <typename... DecayedArgs>
  static void destoy_tuple(std::byte* buffer) {
    auto* tuple_ptr =
        std::launder(reinterpret_cast<std::tuple<DecayedArgs...>*>(buffer));

    std::destroy_at(tuple_ptr);
  }

  template <typename... DecayedArgs>
  static void move_tuple(std::byte* dest, std::byte* src) {
   auto* src_tuple_ptr =
       std::launder(reinterpret_cast<std::tuple<DecayedArgs...>*>(src));

    std::construct_at(reinterpret_cast<std::tuple<DecayedArgs...>*>(dest),
                     std::move(*src_tuple_ptr));
 }
};

}  // namespace log_library::internal
