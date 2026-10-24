#pragma once
// Owner-independent admission policy. Never schedules or executes retries.
#include <cstdint>
#include <limits>

namespace axv {
enum class NativeRetryAdmission {
    Allowed, WaitForDeadline, Exhausted, TimeRegressed
};

class NativeRegistrationRetryBudget {
public:
    NativeRetryAdmission check(std::uint64_t now_ms) const noexcept {
        if (have_clock_ && now_ms < last_clock_ms_) return NativeRetryAdmission::TimeRegressed;
        if (failures_ >= kMaxFailures) return NativeRetryAdmission::Exhausted;
        if (have_deadline_ && now_ms < next_allowed_ms_)
            return NativeRetryAdmission::WaitForDeadline;
        return NativeRetryAdmission::Allowed;
    }
    // A failure fences the *current attempt*. A separate operator action and
    // fresh host generation are required before any next registration request.
    void record_failed_attempt(std::uint64_t now_ms) noexcept {
        if (have_clock_ && now_ms < last_clock_ms_) {
            failures_ = kMaxFailures;
            return;
        }
        have_clock_ = true;
        last_clock_ms_ = now_ms;
        if (failures_ < kMaxFailures) ++failures_;
        const std::uint64_t delay = (std::uint64_t{1} << (failures_ - 1)) * 1000;
        next_allowed_ms_ = now_ms > std::numeric_limits<std::uint64_t>::max() - delay
            ? std::numeric_limits<std::uint64_t>::max() : now_ms + delay;
        have_deadline_ = true;
    }
    // New user consent / reconfiguration can reset the local budget, but a
    // retry must still create a new native session and pass SDK gates again.
    void reset_with_explicit_user_action() noexcept {
        failures_ = 0;
        last_clock_ms_ = 0;
        next_allowed_ms_ = 0;
        have_clock_ = false;
        have_deadline_ = false;
    }
    std::uint32_t failures() const noexcept { return failures_; }
    std::uint64_t next_allowed_ms() const noexcept {
        return have_deadline_ ? next_allowed_ms_ : 0;
    }
    static constexpr std::uint32_t kMaxFailures = 5;
private:
    std::uint64_t last_clock_ms_ = 0, next_allowed_ms_ = 0;
    std::uint32_t failures_ = 0;
    bool have_clock_ = false, have_deadline_ = false;
};
} // namespace axv
