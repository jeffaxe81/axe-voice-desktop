#pragma once
#include <cstdint>

namespace axv {
// Pure policy, no Windows dependency. The caller owns the process and timeout action.
class ShutdownDeadline {
public:
    bool begin(std::uint64_t now_ms) {
        if (pending_) return false;
        started_at_ = now_ms;
        pending_ = true;
        return true;
    }
    bool pending() const { return pending_; }
    bool expired(std::uint64_t now_ms, std::uint64_t timeout_ms) const {
        return pending_ && (now_ms - started_at_) >= timeout_ms;
    }
    void clear() { pending_ = false; started_at_ = 0; }
private:
    std::uint64_t started_at_ = 0;
    bool pending_ = false;
};
