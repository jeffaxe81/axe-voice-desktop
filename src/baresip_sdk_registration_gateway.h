#pragma once
#include "baresip_sdk_event_adapter.h"
#include "baresip_registration_watchdog.h"
#include "baresip_native_thread_guard.h"
#include <cstdint>

namespace axv {
enum class NativeRegistrationSubmission {
    InvalidAccount, BridgeUnavailable, SubmissionRejected,
    Submitted, AlreadySubmitted, NativeSessionRevoked, WrongEventLoopThread,
    RegistrarRejectedSynchronously
};

// Experimental native-event-loop-thread-only registration submission.
// ua_register's success DOES NOT prove authentication/registration.
class BaresipSdkRegistrationGateway {
public:
    explicit BaresipSdkRegistrationGateway(BaresipSdkEventAdapter& bridge)
        : bridge_(bridge) {}
    BaresipSdkRegistrationGateway(const BaresipSdkRegistrationGateway&) = delete;
    BaresipSdkRegistrationGateway& operator=(const BaresipSdkRegistrationGateway&) = delete;
    // External owner must call stop() on the serialized event-loop thread
    // before releasing this gateway or destroying its selected UA.

    NativeRegistrationSubmission submit(struct ua* selected) {
        if (!thread_guard_.is_owner())
            return NativeRegistrationSubmission::WrongEventLoopThread;
        if (owner_) {
            // A shutdown may already have invalidated/destroyed the UA.
            // Require explicit stop() to clear the stale non-owning pointer.
            return bridge_.active() ? NativeRegistrationSubmission::AlreadySubmitted
                                    : NativeRegistrationSubmission::NativeSessionRevoked;
        }
        if (!selected) return NativeRegistrationSubmission::InvalidAccount;
        if (!bridge_.start(selected))
            return NativeRegistrationSubmission::BridgeUnavailable;
        // A native BEVENT_REGISTERING event may be emitted synchronously.
        const int error = ua_register(selected);
        if (error) {
            bridge_.stop(); // Fail closed and invalidate pending events.
            return NativeRegistrationSubmission::SubmissionRejected;
        }
        // ua_register() may synchronously emit an error, shutdown or global
        // exit callback and STILL return zero. Do not retain an invalid UA
        // or claim a live submission after its callback generation was revoked.
        if (!bridge_.active()) {
            bridge_.stop();
            return NativeRegistrationSubmission::NativeSessionRevoked;
        }
        // A Baresip REGISTER_FAIL may fire *inside* ua_register(), even
        // when that function itself returns zero. Surface the exact cause
        // without temporarily admitting a rejected registration.
        if (bridge_.native_register_fail_seen()) {
            bridge_.stop();
            return NativeRegistrationSubmission::RegistrarRejectedSynchronously;
        }
        owner_ = selected;
        confirmation_observed_ = false;
        deadline_.clear(); // Never inherit a previous registration's deadline.
        return NativeRegistrationSubmission::Submitted;
    }

    // Opt-in watchdog. The native event-loop owner supplies monotonic
    // milliseconds and calls poll_registration_deadline() on the SAME thread.
    // It does not run automatically, start a timer, or enable GUI dialing.
    bool arm_registration_deadline(std::uint64_t now_ms,
                                   std::uint64_t max_wait_ms) {
        return thread_guard_.is_owner() && submitted() &&
               !bridge_.native_register_fail_seen() &&
               deadline_.arm(session(), now_ms, max_wait_ms);
    }
    RegistrationDeadlineVerdict poll_registration_deadline(std::uint64_t now_ms) {
        if (!thread_guard_.is_owner())
            return RegistrationDeadlineVerdict::WrongEventLoopThread;
        // Detect explicit registrar rejection immediately: waiting for the
        // timeout would leave a rejected UA live longer than necessary.
        if (bridge_.native_register_fail_seen()) {
            stop();
            return RegistrationDeadlineVerdict::RegistrarRejected;
        }
        // The watchdog deadline may have been cleared after REGISTER_OK.
        // A later lost UA/session must still fence native registration;
        // the earlier confirmation can never authorize persistent status.
        if (confirmation_observed_ && !bridge_.native_register_ok_seen()) {
            stop();
            return RegistrationDeadlineVerdict::SessionRevoked;
        }
        const auto decision = deadline_.check(bridge_.session(), now_ms,
                                             bridge_.native_register_ok_seen());
        switch (decision) {
        case RegistrationDeadlineVerdict::CallbackConfirmed:
            confirmation_observed_ = true;
            deadline_.clear();
            break;
        case RegistrationDeadlineVerdict::Expired:
        case RegistrationDeadlineVerdict::ClockRegressed:
        case RegistrationDeadlineVerdict::SessionRevoked:
            stop(); // Fail closed: unregister only while UA still live.
            break;
        default:
            break;
        }
        return decision;
    }
    // False means WRONG THREAD; nothing is touched and the native-loop
    // owner remains responsible for an eventual safe stop(). No implicit
    // cross-thread request or deferred cleanup is scheduled by this class.
    bool stop() {
        if (!thread_guard_.is_owner()) return false;
        deadline_.clear();
        confirmation_observed_ = false;
        if (owner_) {
            // Terminal events can invalidate/destroy the selected UA.
            // Skip native dereference when the owning epoch is revoked.
            // An event-loop owner MUST guarantee UA lifetime while active.
            if (bridge_.active()) ua_unregister(owner_);
            owner_ = nullptr;
        }
        bridge_.stop(); // Revoke events and unregister the callback.
        return true;
    }

    bool submitted() const {
        return thread_guard_.is_owner() && owner_ != nullptr && bridge_.active() &&
               !bridge_.native_register_fail_seen();
    }
    bool registrar_rejection_seen() const {
        return thread_guard_.is_owner() && bridge_.native_register_fail_seen();
    }
    // Session identity does not mean the registrar accepted credentials.
    std::uint64_t session() const { return submitted() ? bridge_.session() : 0; }
private:
    BaresipSdkEventAdapter& bridge_;
    BaresipNativeThreadGuard thread_guard_{};
    struct ua* owner_ = nullptr;
    RegistrationDeadline deadline_{};
    bool confirmation_observed_ = false; // native owner thread only
};
} // namespace axv
