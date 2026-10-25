#pragma once
#include <cstdint>

namespace axv {

// DBT_DEVNODES_CHANGED (7), DBT_CONFIGCHANGED (0x18),
// DBT_DEVICEARRIVAL (0x8000), DBT_DEVICEREMOVECOMPLETE (0x8004). These are generic Windows device
// notifications, not evidence that an audio device really changed.
inline bool is_potential_device_change(unsigned event_code) {
    return event_code == 7u || event_code == 0x18u ||
           event_code == 0x8000u || event_code == 0x8004u;
}

// UI-thread-only debounce: coalesce hotplug storms, do not enumerate hardware
// while UI is hidden or in a different tab. No SIP/RTP actions are performed.
class AudioHotplugPolicy {
public:
    static constexpr std::uint64_t kQuietPeriodMs = 450;
    bool notify(unsigned code, std::uint64_t monotonic_ms) {
        if (!is_potential_device_change(code)) return false;
        pending_ = true;
        last_change_ms_ = monotonic_ms;
        return true;
    }
    bool take_if_due(std::uint64_t monotonic_ms, bool audio_tab_visible) {
        if (!pending_ || !audio_tab_visible || monotonic_ms < last_change_ms_ ||
            monotonic_ms - last_change_ms_ < kQuietPeriodMs)
            return false;
        pending_ = false;
        return true;
    }
    void acknowledge_manual_refresh() { pending_ = false; }
    bool pending() const { return pending_; }

private:
    bool pending_ = false;
    std::uint64_t last_change_ms_ = 0;
};
} // namespace axv
