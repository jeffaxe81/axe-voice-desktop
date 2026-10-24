#pragma once
#include "baresip_event_reducer.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace axv {
// Native Baresip callback thread -> UI thread. No credentials/URIs in events.
// Events are NOT authenticated by this queue; reducer gates provenance.
class BaresipEventMailbox {
public:
    static constexpr std::size_t kCapacity = 32;
    struct Batch {
        std::array<BaresipNativeEvent, kCapacity> events{};
        std::size_t size = 0;
        std::uint64_t session = 0;
        bool overflow = false;
    };
    std::uint64_t begin_session() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (generation_ == UINT64_MAX) {
            session_ = 0; overflow_ = true; pending_ = 0;
            return 0; // Never reuse a session ID.
        }
        session_ = ++generation_;
        pending_ = 0;
        first_ = 0;
        overflow_ = false;
        return session_;
    }
    void invalidate_session() {
        std::lock_guard<std::mutex> lock(mutex_);
        session_ = 0;
        pending_ = 0;
        first_ = 0;
        overflow_ = false;
    }
    bool enqueue(BaresipNativeEvent event) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (session_ == 0 || event.session != session_ || overflow_) return false;
        if (pending_ == kCapacity) {
            pending_ = 0; first_ = 0; overflow_ = true;
            return false; // Drop ALL events on overflow, never a partial history.
        }
        entries_[(first_ + pending_) % kCapacity] = event;
        ++pending_;
        return true;
    }
    Batch drain() {
        std::lock_guard<std::mutex> lock(mutex_);
        Batch batch;
        batch.session = session_;
        batch.overflow = overflow_;
        if (!overflow_) {
            for (std::size_t i = 0; i < pending_; ++i)
                batch.events[i] = entries_[(first_ + i) % kCapacity];
            batch.size = pending_;
        }
        first_ = pending_ = 0;
        return batch;
    }
private:
    std::mutex mutex_;
    std::array<BaresipNativeEvent, kCapacity> entries_{};
    std::size_t pending_ = 0;
    std::size_t first_ = 0;
    std::uint64_t generation_ = 0;
    std::uint64_t session_ = 0;
    bool overflow_ = false;
};
} // namespace axv
