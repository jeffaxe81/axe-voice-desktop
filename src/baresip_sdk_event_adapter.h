#pragma once
// Optional, isolated laboratory bridge. NOT part of the AXEVoice executable.
// Production requires the audited Baresip and libre SDKs and an event-loop
// owner that serializes callbacks with stop() / unregister completion.
#include <baresip.h>
#include "baresip_callback_lifecycle.h"
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <limits>
#include <memory>
#include <vector>
#include <unordered_set>

namespace axv {

// Baresip's public bevent callback API -> the AXE Voice session-fenced mailbox.
// Registration is scope-limited to ONE selected UA and ONE active call.
// No passwords, SIP URIs, raw pointers, or IDs derived from pointers enter events.
// This does not issue VerifiedNativeApi and does not implement SipEngine.
class BaresipSdkEventAdapter {
public:
    explicit BaresipSdkEventAdapter(BaresipCallbackLifecycle& lifecycle)
        : lifecycle_(lifecycle) {}
    BaresipSdkEventAdapter(const BaresipSdkEventAdapter&) = delete;
    BaresipSdkEventAdapter& operator=(const BaresipSdkEventAdapter&) = delete;
    ~BaresipSdkEventAdapter() { stop(); }

    // Must be called from the Baresip event-loop owner thread.
    // Caller MUST await all in-flight callbacks before stop() and destruction.
    bool start(struct ua* owned_ua) {
        if (registered_ || !owned_ua || contexts_.size() >= kMaxContexts) return false;
        // Baresip's global bevent_register() first calls
        // bevent_unregister(handler): two instances using this static callback
        // would silently replace each other's subscription. Claim ownership
        // BEFORE touching the Baresip event registry or lifecycle.
        BaresipSdkEventAdapter* expected = nullptr;
        if (!process_owner_.compare_exchange_strong(expected, this))
            return false;
        const auto session = lifecycle_.begin();
        if (!session) {
            release_owner();
            return false;
        }
        owner_ = owned_ua;
        captured_ = session;
        native_key_ = 0;
        active_call_ = nullptr;
        seen_calls_.clear();
        // Each native registration receives a *distinct immutable epoch*.
        // Contexts remain allocated until owner-proven callback quiescence.
        // A failed allocation must not leak its context or monopolize the
        // process-wide bevent registration claim.
        try {
            auto context = std::unique_ptr<CallbackContext>(
                new CallbackContext{this, session});
            contexts_.push_back(std::move(context));
        } catch (...) {
            lifecycle_.revoke_and_wait();
            owner_ = nullptr;
            captured_ = {};
            release_owner();
            return false;
        }
        registered_ = true;
        const int error = bevent_register(&BaresipSdkEventAdapter::callback,
                                          contexts_.back().get());
        if (error) {
            registered_ = false;
            lifecycle_.revoke_and_wait();
            owner_ = nullptr;
            captured_ = {};
            release_owner();
            return false;
        }
        return true;
    }

    void stop() {
        // Event-loop contract: no concurrent/in-flight SDK callbacks here.
        if (registered_) bevent_unregister(&BaresipSdkEventAdapter::callback);
        registered_ = false;
        lifecycle_.revoke_and_wait();
        owner_ = nullptr;
        captured_ = {};
        native_key_ = 0;
        active_call_ = nullptr;
        seen_calls_.clear();
        // Only release the process-wide claim AFTER native unregistration
        // and callback admission quiescence, never while subscribed.
        release_owner();
    }
    bool active() const {
        return registered_ && captured_ && lifecycle_.is_live(captured_);
    }
    std::uint64_t session() const {
        return active() ? captured_.mailbox_session : 0;
    }

private:
    // One owner for this SDK event-handler entry point. The upstream native
    // registry is keyed by the FUNCTION POINTER, not its context argument.
    static inline std::atomic<BaresipSdkEventAdapter*> process_owner_{nullptr};
    void release_owner() {
        BaresipSdkEventAdapter* expected = this;
        (void)process_owner_.compare_exchange_strong(expected, nullptr);
    }

    struct CallbackContext {
        BaresipSdkEventAdapter* owner;
        BaresipCallbackLifecycle::CapturedSession captured;
    };
    static constexpr std::size_t kMaxContexts = 512;

    static void callback(enum bevent_ev type, struct bevent* event, void* arg) {
        auto* context = static_cast<CallbackContext*>(arg);
        if (context && context->owner)
            context->owner->on_event(context->captured, type, event);
    }

    bool same_epoch(BaresipCallbackLifecycle::CapturedSession captured) const {
        return captured.fence_generation == captured_.fence_generation &&
               captured.mailbox_session == captured_.mailbox_session;
    }

    void fail_closed() {
        // Called from SDK callback: do not wait for callbacks here.
        lifecycle_.reject_current();
        captured_ = {}; // explicit stop() required before restart
        active_call_ = nullptr;
    }

    void on_event(BaresipCallbackLifecycle::CapturedSession callback_session,
                  enum bevent_ev type, struct bevent* event) {
        if (!same_epoch(callback_session) || !active()) return;
        // BEVENT_EXIT is global. BEVENT_SHUTDOWN is emitted for a specific
        // UA (including from ua_destroy), and MUST pass the ownership check.
        // Both terminal events revoke proof and queued calls immediately.
        if (type == BEVENT_EXIT) {
            fail_closed();
            return;
        }
        // Upstream emits SIPSESS_CONN/FAILED with bevent_sip_msg_emit:
        // those are message events, NOT UA/call events. A rejected INVITE
        // cannot be attributed to this UA through bevent_get_ua().
        // Nor may foreign/unsupported event types be read as UA events.
        switch (type) {
        case BEVENT_REGISTERING:
        case BEVENT_REGISTER_OK:
        case BEVENT_REGISTER_FAIL:
        case BEVENT_UNREGISTERING:
        case BEVENT_SHUTDOWN:
        case BEVENT_AUDIO_ERROR:
        case BEVENT_CALL_INCOMING:
        case BEVENT_CALL_OUTGOING:
        case BEVENT_CALL_ESTABLISHED:
        case BEVENT_CALL_CLOSED:
            break;
        default:
            return;
        }
        if (!event || bevent_get_ua(event) != owner_) return;
        if (type == BEVENT_SHUTDOWN) {
            fail_closed();
            return;
        }
        switch (type) {
        case BEVENT_AUDIO_ERROR:
            // Signaling/media failures must not leave a usable call state.
            // Only errors from the selected UA are allowed to revoke it.
            fail_closed();
            return;
        case BEVENT_REGISTERING:
            if (!lifecycle_.registration(callback_session, BaresipEventKind::Registering))
                fail_closed();
            return;
        case BEVENT_REGISTER_OK:
            if (!lifecycle_.registration(callback_session, BaresipEventKind::RegisterOk))
                fail_closed();
            return;
        case BEVENT_REGISTER_FAIL:
            if (!lifecycle_.registration(callback_session, BaresipEventKind::RegisterFail))
                fail_closed();
            return;
        case BEVENT_UNREGISTERING:
            if (!lifecycle_.registration(callback_session, BaresipEventKind::Unregistering))
                fail_closed();
            return;
        case BEVENT_CALL_INCOMING:
        case BEVENT_CALL_OUTGOING: {
            const struct call* call = bevent_get_call(event);
            // Never accept pointer reuse in the same session. Otherwise a late
            // callback could confuse a new call with a previously closed one.
            if (!call || active_call_ || seen_calls_.count(call) ||
                seen_calls_.size() >= 4096 ||
                native_key_ == std::numeric_limits<std::uint64_t>::max()) {
                fail_closed();
                return;
            }
            const std::uint64_t new_key = native_key_ + 1;
            const auto kind = type == BEVENT_CALL_INCOMING
                ? BaresipEventKind::CallIncoming : BaresipEventKind::CallOutgoing;
            if (!lifecycle_.call_started(callback_session, new_key, kind)) {
                fail_closed();
                return;
            }
            native_key_ = new_key;
            active_call_ = call;
            seen_calls_.insert(call); // never queued, logged or persisted
            return;
        }
        case BEVENT_CALL_ESTABLISHED:
        case BEVENT_CALL_CLOSED: {
            const struct call* call = bevent_get_call(event);
            if (!call || call != active_call_) {
                fail_closed();
                return;
            }
            const auto kind = type == BEVENT_CALL_ESTABLISHED
                ? BaresipEventKind::CallEstablished : BaresipEventKind::CallClosed;
            if (!lifecycle_.call_changed(callback_session, native_key_, kind)) {
                fail_closed();
                return;
            }
            if (type == BEVENT_CALL_CLOSED) active_call_ = nullptr;
            return;
        }
        default:
            // Unsupported Baresip events are not evidence of registration or
            // media security, and must not mutate AXE Voice's call state.
            return;
        }
    }

    BaresipCallbackLifecycle& lifecycle_;
    struct ua* owner_ = nullptr;
    const struct call* active_call_ = nullptr;
    std::unordered_set<const struct call*> seen_calls_;
    BaresipCallbackLifecycle::CapturedSession captured_{};
    // Never deallocate per-epoch callback arguments across restarts.
    // The containing owner must ensure SDK callback quiescence before destruction.
    std::vector<std::unique_ptr<CallbackContext>> contexts_;
    std::uint64_t native_key_ = 0;
    bool registered_ = false;
};
} // namespace axv
