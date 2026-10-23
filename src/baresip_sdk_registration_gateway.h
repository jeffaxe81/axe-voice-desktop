#pragma once
#include "baresip_sdk_event_adapter.h"
#include <cstdint>

namespace axv {
enum class NativeRegistrationSubmission {
    InvalidAccount, BridgeUnavailable, SubmissionRejected,
    Submitted, AlreadySubmitted
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
        if (owner_) return NativeRegistrationSubmission::AlreadySubmitted;
        if (!selected) return NativeRegistrationSubmission::InvalidAccount;
        if (!bridge_.start(selected))
            return NativeRegistrationSubmission::BridgeUnavailable;
        // A native BEVENT_REGISTERING event may be emitted synchronously.
        const int error = ua_register(selected);
        if (error) {
            bridge_.stop(); // Fail closed and invalidate pending events.
            return NativeRegistrationSubmission::SubmissionRejected;
        }
        owner_ = selected;
        return NativeRegistrationSubmission::Submitted;
    }

    void stop() {
        if (owner_) {
            // Must remain alive during ua_unregister's native callbacks.
            ua_unregister(owner_);
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
