#pragma once

#include <atomic>

namespace axv {

// One atomic state makes close terminal even when a callback is in flight.
// Each observer owns a separate signal; retained callbacks keep only that
// observer's signal alive through shared_ptr ownership.
class AudioDeviceChangeSignal {
public:
    void notify() noexcept {
        unsigned expected = clean;
        state_.compare_exchange_strong(expected, dirty, std::memory_order_relaxed);
    }

    bool take() noexcept {
        unsigned expected = dirty;
        return state_.compare_exchange_strong(expected, clean, std::memory_order_relaxed);
    }

    void close() noexcept {
        state_.store(closed, std::memory_order_relaxed);
    }

private:
    static constexpr unsigned clean = 0;
    static constexpr unsigned dirty = 1;
    static constexpr unsigned closed = 2;
    static_assert(std::atomic<unsigned>::is_always_lock_free,
                  "Audio notification callbacks require lock-free atomics");
    std::atomic<unsigned> state_{clean};
};

} // namespace axv
