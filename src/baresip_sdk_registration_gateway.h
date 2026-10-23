#pragma once
#include "baresip_sdk_event_adapter.h"
#include <cstdint>

namespace axv {
enum class NativeRegistrationSubmission {
    InvalidAccount, BridgeUnavailable, SubmissionRejected,
    Submitted, AlreadySubmitted, NativeSessionRevoked
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
        owner_ = selected;
        return NativeRegistrationSubmission::Submitted;
    }

    void stop() {
        if (owner_) {
            // Terminal events can invalidate/destroy the selected UA.
            // Skip native dereference when the owning epoch is revoked.
            // An event-loop owner MUST guarantee UA lifetime while active.
            if (bridge_.active()) ua_unregister(owner_);
            owner_ = nullptr;
        }
        bridge_.stop(); // Revoke events and unregister the callback.
    }

    bool submitted() const { return owner_ != nullptr && bridge_.active(); }
    // Session identity does not mean the registrar accepted credentials.
    std::uint64_t session() const { return submitted() ? bridge_.session() : 0; }
private:
    BaresipSdkEventAdapter& bridge_;
    struct ua* owner_ = nullptr;
};
} // namespace axv
