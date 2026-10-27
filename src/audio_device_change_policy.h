#pragma once

#include "audio_inventory_async.h"
#include <cstdint>
#include <limits>

namespace axv {

inline bool is_audio_device_change(std::uintptr_t kind) {
    return kind == 0x8000 || kind == 0x8004 || kind == 0x0007 || kind == 0x0018;
}

class AudioDeviceChangePolicy {
public:
    void enter(std::uint64_t now_ms) {
        active_ = true;
        dirty_ = false;
        last_ms_ = now_ms;
        periodic_ms_ = deadline(now_ms, 5000);
    }

    void leave() {
        active_ = false;
        dirty_ = false;
    }

    void changed(std::uint64_t now_ms) {
        if (!active_) return;
        dirty_ = true;
        dirty_ms_ = deadline(observe(now_ms), 250);
    }

    bool should_refresh(std::uint64_t now_ms, bool dropdown_open) const {
        return active_ && !dropdown_open &&
               ((dirty_ && now_ms >= dirty_ms_) || now_ms >= periodic_ms_);
    }

    // Pace fallback from completion without erasing a later dirty event.
    void completed(std::uint64_t now_ms) {
        if (!active_) return;
        periodic_ms_ = deadline(observe(now_ms), 5000);
    }

    void refreshed(std::uint64_t now_ms) {
        if (!active_) return;
        dirty_ = false;
        periodic_ms_ = deadline(observe(now_ms), 5000);
    }

private:
    static std::uint64_t deadline(std::uint64_t now, std::uint64_t delay) {
        constexpr auto max = std::numeric_limits<std::uint64_t>::max();
        return now > max - delay ? max : now + delay;
    }

    // A clock regression must never bring an existing deadline forward.
    std::uint64_t observe(std::uint64_t now_ms) {
        if (now_ms > last_ms_) last_ms_ = now_ms;
        return last_ms_;
    }

    bool active_ = false;
    bool dirty_ = false;
    std::uint64_t last_ms_ = 0;
    std::uint64_t dirty_ms_ = 0;
    std::uint64_t periodic_ms_ = 0;
};

// Do not replace a slow or ready result merely because fallback time elapsed.
inline bool audio_device_refresh_due(const AudioDeviceChangePolicy& policy,
    std::uint64_t now_ms,bool dropdown_open,const AudioInventoryAsyncSnapshot& inventory) {
    return !inventory.closed && !inventory.busy && !inventory.pending && !inventory.ready &&
           policy.should_refresh(now_ms,dropdown_open);
}

} // namespace axv
