#pragma once
#include <cstdint>
namespace axv {
// Pure GUI-thread-only debounce: no audio identifiers, credentials or I/O.
class AudioHotplugDebounce {
public:
    static constexpr std::uint64_t kQuietPeriodMs = 400;
    void notify(std::uint64_t time_ms) { pending_=true; last_=time_ms; }
    bool pending() const { return pending_; }
    void clear() { pending_=false; }
    bool consume_if_due(std::uint64_t now_ms, bool visible) {
        if (!pending_ || !visible || now_ms < last_ ||
            now_ms-last_ < kQuietPeriodMs) return false;
        pending_=false;
        return true;
    }
private:
    bool pending_=false;
    std::uint64_t last_=0;
};
} // namespace axv
