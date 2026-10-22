#pragma once
// Optional, isolated laboratory bridge. NOT part of the AXEVoice executable.
// Production requires the audited Baresip and libre SDKs and an event-loop
// owner that serializes callbacks with stop() / unregister completion.
#include <baresip.h>
#include "baresip_callback_ingress.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_set>

namespace axv {

// Baresip's public bevent callback API -> the AXE Voice session-fenced mailbox.
// Registration is scope-limited to ONE selected UA and ONE active call.
// No passwords, SIP URIs, raw pointers, or IDs derived from pointers enter events.
// This does not issue VerifiedNativeApi and does not implement SipEngine.
class BaresipSdkEventAdapter {
public:
    explicit BaresipSdkEventAdapter(BaresipCallbackIngress& ingress)
        : ingress_(ingress) {}
    BaresipSdkEventAdapter(const BaresipSdkEventAdapter&) = delete;
    BaresipSdkEventAdapter& operator=(const BaresipSdkEventAdapter&) = delete;
    ~BaresipSdkEventAdapter() { stop(); }

    // Must be called from the Baresip event-loop owner thread.
    // Caller MUST await all in-flight callbacks before stop() and destruction.
    bool start(struct ua* owned_ua) {
        if (registered_ || !owned_ua || ingress_.session() != 0) return false;
        const auto session = ingress_.begin_session();
        if (session == 0) return false;
        owner_ = owned_ua;
        session_ = session;
        native_key_ = 0;
        active_call_ = nullptr;
        seen_calls_.clear();
        registered_ = true;
        const int error = bevent_register(&BaresipSdkEventAdapter::callback, this);
        if (error) {
            registered_ = false;
            ingress_.revoke();
            owner_ = nullptr;
            session_ = 0;
            return false;
        }
        return true;
    }

    void stop() {
        // Event-loop contract: no concurrent/in-flight SDK callbacks here.
        if (registered_) bevent_unregister(&BaresipSdkEventAdapter::callback);
        registered_ = false;
        ingress_.revoke();
        owner_ = nullptr;
        session_ = 0;
        native_key_ = 0;
        active_call_ = nullptr;
        seen_calls_.clear();
    }
    bool active() const {
        return registered_ && session_ != 0 && ingress_.session() == session_;
    }
    std::uint64_t session() const { return active() ? session_ : 0; }

private:
    static void callback(enum bevent_ev type, struct bevent* event, void* arg) {
        if (arg) static_cast<BaresipSdkEventAdapter*>(arg)->on_event(type, event);
    }

    void fail_closed() {
        ingress_.revoke();
        session_ = 0; // cannot recover until explicit stop and start
        active_call_ = nullptr;
    }

    void on_event(enum bevent_ev type, struct bevent* event) {
        if (!active()) return;
        // BEVENT_EXIT is global. BEVENT_SHUTDOWN is emitted for a specific
        // UA (including from ua_destroy), and MUST pass the ownership check.
        // Both terminal events revoke proof and queued calls immediately.
        if (type == BEVENT_EXIT) {
            fail_closed();
            return;
        }
        if (!event || bevent_get_ua(event) != owner_) return;
        if (type == BEVENT_SHUTDOWN) {
            fail_closed();
            return;
        }
        switch (type) {
        case BEVENT_SIPSESS_FAILED:
        case BEVENT_AUDIO_ERROR:
            // Signaling/media failures must not leave a usable call state.
            // Only errors from the selected UA are allowed to revoke it.
            fail_closed();
            return;
        case BEVENT_REGISTERING:
            if (!ingress_.registration(session_, BaresipEventKind::Registering))
                fail_closed();
            return;
        case BEVENT_REGISTER_OK:
            if (!ingress_.registration(session_, BaresipEventKind::RegisterOk))
                fail_closed();
            return;
        case BEVENT_REGISTER_FAIL:
            if (!ingress_.registration(session_, BaresipEventKind::RegisterFail))
                fail_closed();
            return;
        case BEVENT_UNREGISTERING:
            if (!ingress_.registration(session_, BaresipEventKind::Unregistering))
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
            if (!ingress_.call_started(session_, new_key, kind)) {
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
            if (!ingress_.call_changed(session_, native_key_, kind)) {
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

    BaresipCallbackIngress& ingress_;
    struct ua* owner_ = nullptr;
    const struct call* active_call_ = nullptr;
    std::unordered_set<const struct call*> seen_calls_;
    std::uint64_t session_ = 0;
    std::uint64_t native_key_ = 0;
    bool registered_ = false;
};
} // namespace axv
