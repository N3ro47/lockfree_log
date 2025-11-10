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
    // Load head with acquire to synchronize with other producers
    auto head = m_head.load(std::memory_order_acquire);

    for (;;) { // Loop until we succeed or determine the queue is full
      const size_t index = head & MASK;
      
      // Load turnstile with acquire to synchronize with the consumer's release
      const size_t turn = m_turnstile[index].load(std::memory_order_acquire);

      // If turn == head, the slot is empty and ready for us.
      if (turn == head) {
        // Try to claim this slot by incrementing head.
        // The weak variant is fine because we are in a loop.
        if (m_head.compare_exchange_weak(head, head + 1, std::memory_order_release)) {
          // We successfully claimed the slot. Now we can write.
          std::construct_at(std::launder(reinterpret_cast<T*>(&m_buffer[index])),
                            std::forward<Args>(args)...);
          // Publish our write by setting the turnstile to head + 1.
          // This release-stores the data and updates the turnstile atomically.
          m_turnstile[index].store(head + 1, std::memory_order_release);
          return true;
        }
        // If CAS failed, another producer beat us. 'head' is now updated to the new
        // value, so we loop and retry for the *next* available slot.
      } else {
        // The slot is not ready for this 'head' value.
        // It could be because the queue is full (turn < head) or another
        // producer is slow (also turn < head).
        // We need to check if another producer has already advanced head.
        auto current_head = m_head.load(std::memory_order_acquire);
        if (current_head == head) {
          // Head hasn't changed, so the queue is genuinely full.
          return false;
        }
        // Another producer has moved head forward. Update our local 'head'
        // and retry the loop.
        head = current_head;
      }
    }
  }

  // The MPSC-optimized try_pop is correct.
  bool try_pop(T& value) {
    auto tail = m_tail.load(std::memory_order_relaxed);
    const size_t index = tail & MASK;

    const size_t turn = m_turnstile[index].load(std::memory_order_acquire);

    if (turn == tail + 1) {
      T* slot = std::launder(reinterpret_cast<T*>(&m_buffer[index]));
      value = *slot;
      std::destroy_at(slot);

      m_turnstile[index].store(tail + Capacity, std::memory_order_release);

      // Using release here ensures that the write to the turnstile above is visible
      // before any other thread sees the updated tail.
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