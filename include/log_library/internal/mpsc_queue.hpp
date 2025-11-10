#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <utility>

constexpr static size_t CACHE_LINE_SIZE = 64;

template <typename T, size_t Capacity>
class MPSCQueue {
 public:
  static_assert((Capacity > 0) && ((Capacity & (Capacity - 1)) == 0),
                "Capacity must be a power of 2");

  MPSCQueue() {
    for (size_t i = 0; i < Capacity; i++) {
      m_turnstile[i].store(i, std::memory_order_relaxed);
    }
  };

  ~MPSCQueue() {
    T dummy;
    while (try_pop(dummy)) {
    }
  }

  MPSCQueue(const MPSCQueue&) = delete;
  MPSCQueue& operator=(const MPSCQueue&) = delete;

  template <typename... Args>
  bool try_emplace(Args&&... args) {
    auto head = m_head.load(std::memory_order_acquire);

    for (;;) {
      const size_t index = head & MASK;

      const size_t turn = m_turnstile[index].load(std::memory_order_acquire);

      if (turn == head) {
        if (m_head.compare_exchange_weak(head, head + 1,
                                         std::memory_order_release)) {
          std::construct_at(
              std::launder(reinterpret_cast<T*>(&m_buffer[index])),
              std::forward<Args>(args)...);
          m_turnstile[index].store(head + 1, std::memory_order_release);
          return true;
        }
      } else {
        auto current_head = m_head.load(std::memory_order_acquire);
        if (current_head == head) {
          return false;
        }
        head = current_head;
      }
    }
  }

  bool try_pop(T& value) {
    auto tail = m_tail.load(std::memory_order_relaxed);
    const size_t index = tail & MASK;

    const size_t turn = m_turnstile[index].load(std::memory_order_acquire);

    if (turn == tail + 1) {
      T* slot = std::launder(reinterpret_cast<T*>(&m_buffer[index]));
      value = *slot;
      std::destroy_at(slot);

      m_turnstile[index].store(tail + Capacity, std::memory_order_release);
      m_tail.store(tail + 1, std::memory_order_release);

      return true;
    }

    return false;
  }

 private:
  static constexpr size_t MASK = Capacity - 1;
  using Storage = alignas(T) std::byte[sizeof(T)];

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_head{0};
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_tail{0};
  std::atomic<std::size_t> m_turnstile[Capacity];
  alignas(CACHE_LINE_SIZE) Storage m_buffer[Capacity];
};