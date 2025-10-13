#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

constexpr static size_t CACHE_LINE_SIZE = 64;

template <typename T, size_t Capacity>
class MPSCQueue {
 public:
  static_assert((Capacity > 0) && ((Capacity & (Capacity - 1)) == 0),
                "Capacity must be a power of 2");

  MPSCQueue() = default;

  ~MPSCQueue() {
    T dummy;
    while (try_pop(dummy)) {
    }
  }

  MPSCQueue(const MPSCQueue &) = delete;
  MPSCQueue &operator=(const MPSCQueue &) = delete;

  template <typename... Args>
  bool try_emplace(Args &&...args) {
    auto current_head = m_head.load(std::memory_order_relaxed);

    for (;;) {
      auto tail = m_tail.load(std::memory_order_acquire);

      if (current_head - tail >= Capacity) {
        return false;
      }

      if (m_head.compare_exchange_weak(current_head, current_head + 1,
                                       std::memory_order_release,
                                       std::memory_order_relaxed)) {
        break;
      }
    }
    new (&m_buffer[current_head & (Capacity - 1)])
        T(std::forward<Args>(args)...);

    return true;
  }

  bool try_pop(T &value) {
    auto current_tail = m_tail.load(std::memory_order_relaxed);

    auto head = m_head.load(std::memory_order_acquire);

    if (current_tail == head) {
      return false;
    }

    const size_t index = current_tail & (Capacity - 1);

    T *slot = reinterpret_cast<T *>(&m_buffer[index]);

    value = std::move(*slot);

    slot->~T();

    m_tail.store(current_tail + 1, std::memory_order_release);

    return true;
  }

 private:
  using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_head{0};

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_tail{0};

  alignas(CACHE_LINE_SIZE) Storage m_buffer[Capacity];
};
