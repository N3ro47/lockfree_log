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
      m_turnstile[i].store(i, std::memory_order_release);
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
  auto current_head = m_head.load(std::memory_order_relaxed);
  
  do {
    if (current_head >= m_tail.load(std::memory_order_acquire) + Capacity) {
      return false;
    }

  } while (!m_head.compare_exchange_weak(current_head, current_head + 1,
                                          std::memory_order_release,
                                          std::memory_order_relaxed));


  const size_t index = current_head & MASK;
  std::construct_at(reinterpret_cast<T*>(&m_buffer[index]),
                    std::forward<Args>(args)...);


  m_turnstile[index].store(current_head + 1, std::memory_order_release);

  return true;
}

  bool try_pop(T& value) {
    auto current_tail = m_tail.load(std::memory_order_seq_cst);

    const size_t index = current_tail & MASK;

    if (m_turnstile[index].load(std::memory_order_seq_cst) != current_tail + 1) {
      return false;  // Queue is empty
    }

    T* slot = std::launder(reinterpret_cast<T*>(&m_buffer[index]));

    value = std::move(*slot);

    std::destroy_at(slot);

    m_tail.store(current_tail + 1, std::memory_order_seq_cst);

    return true;
  }

 private:
  static constexpr size_t MASK = Capacity - 1;

  using Storage = alignas(T) std::byte[sizeof(T)];

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_head{0};

  alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_tail{0};

  std::atomic<std::size_t> m_turnstile[Capacity];

  alignas(CACHE_LINE_SIZE) Storage m_buffer[Capacity];
};
