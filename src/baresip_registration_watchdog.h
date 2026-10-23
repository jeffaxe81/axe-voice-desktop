#pragma once
#include <cstdint>

namespace axv {
// Deterministic, externally-polled deadline. No global clock or timer thread.
enum class RegistrationDeadlineVerdict {
    Unarmed, Pending, CallbackConfirmed, Expired, ClockRegressed, SessionRevoked,
    WrongEventLoopThread
};
class RegistrationDeadline {
public:
    bool arm(std::uint64_t session, std::uint64_t monotonic_ms,
             std::uint64_t max_wait_ms) {
        if (!session || !max_wait_ms || armed_) return false;
        session_ = session;
        started_ms_ = monotonic_ms;
        max_wait_ms_ = max_wait_ms;
        armed_ = true;
        return true;
    }
    RegistrationDeadlineVerdict check(std::uint64_t session,
                                      std::uint64_t monotonic_ms,
                                      bool native_callback_confirmed) const {
        if (!armed_) return RegistrationDeadlineVerdict::Unarmed;
        if (session != session_ || !session)
            return RegistrationDeadlineVerdict::SessionRevoked;
        // Treat monotonic clock rollback as unsafe, rather than extending a
        // pending registration indefinitely. Unsigned subtraction handles
        // timestamps near UINT64_MAX without deadline-addition overflow.
        if (monotonic_ms < started_ms_)
            return RegistrationDeadlineVerdict::ClockRegressed;
        if (monotonic_ms - started_ms_ >= max_wait_ms_)
            return RegistrationDeadlineVerdict::Expired;
        // A local ua_register() return and ua_isregistered() alone cannot
        // replace the matching native REGISTER_OK callback for this epoch.
        return native_callback_confirmed
            ? RegistrationDeadlineVerdict::CallbackConfirmed
            : RegistrationDeadlineVerdict::Pending;
    }
    void clear() { armed_ = false; session_ = started_ms_ = max_wait_ms_ = 0; }
    bool armed() const { return armed_; }
private:
    bool armed_ = false;
    std::uint64_t session_ = 0;
    std::uint64_t started_ms_ = 0;
    std::uint64_t max_wait_ms_ = 0;
};
} // namespace axv
