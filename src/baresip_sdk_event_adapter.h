#pragma once
// Optional, isolated laboratory bridge. NOT part of the AXEVoice executable.
// Production requires the audited Baresip and libre SDKs and an event-loop
// owner that serializes callbacks with stop() / unregister completion.
#include <baresip.h>
#include <atomic>
#include "baresip_callback_lifecycle.h"
#include <cstddef>
#include <cstdint>
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
        // Each SDK registration receives a unique immutable callback epoch.
        // Retired contexts remain valid until a native event-loop barrier.
        if (registered_ || !owned_ua || contexts_.size() >= kMaxEpochs) return false;
        // bevent_unregister() unregisters by callback FUNCTION address, not
        // by context arg. Enforce one adapter owning this callback per process.
        BaresipSdkEventAdapter* vacant = nullptr;
        if (!registry_owner_.compare_exchange_strong(vacant, this)) return false;
        const auto epoch = lifecycle_.begin();
        if (!epoch) {
            release_registry_owner();
            return false;
        }
        owner_ = owned_ua;
        captured_ = epoch;
        native_key_ = 0;
        active_call_ = nullptr;
        seen_calls_.clear();
        contexts_.emplace_back(new CallbackContext{this, epoch});
        registered_ = true;
        const int error = bevent_register(&BaresipSdkEventAdapter::callback,
                                          contexts_.back().get());
        if (error) {
            registered_ = false;
            lifecycle_.revoke_and_wait();
            owner_ = nullptr;
            captured_ = {};
            release_registry_owner();
            return false;
        }
        return true;
    }

    void stop() {
        // Event-loop contract: no concurrent/in-flight SDK callbacks here.
        if (registered_) bevent_unregister(&BaresipSdkEventAdapter::callback);
        // Keep registered_ and native owner/context stable until every callback
        // which already entered has released its outer lease. Otherwise a
        // concurrent active() read may race against this state change.
        lifecycle_.revoke_and_wait();
        registered_ = false;
        owner_ = nullptr;
        captured_ = {};
        native_key_ = 0;
        active_call_ = nullptr;
        seen_calls_.clear();
        release_registry_owner();
    }
    bool active() const {
        return registered_ && captured_ && lifecycle_.is_live(captured_);
    }
    std::uint64_t session() const {
        return active() ? captured_.mailbox_session : 0;
    }

private:
    // C++17 inline static: one lease across all translation units.
    // External event-loop serialization and callback quiescence remain required.
    inline static std::atomic<BaresipSdkEventAdapter*> registry_owner_{nullptr};
    void release_registry_owner() {
        BaresipSdkEventAdapter* self = this;
        (void)registry_owner_.compare_exchange_strong(self, nullptr);
    }
    struct CallbackContext {
        BaresipSdkEventAdapter* owner;
        BaresipCallbackLifecycle::CapturedSession epoch;
    };
    static constexpr std::size_t kMaxEpochs = 512;

    static void callback(enum bevent_ev type, struct bevent* event, void* arg) {
        auto* context = static_cast<CallbackContext*>(arg);
        if (!context || !context->owner) return;
        // Every dispatched callback holds its epoch alive through on_event().
        // stop() must not release account/call state while this lease exists.
        auto lease = context->owner->lifecycle_.hold_native_callback(context->epoch);
        if (!lease) return;
        context->owner->on_event(context->epoch, type, event);
    }

    void fail_closed() {
        // Called within an SDK callback: must not wait on a callback lease.
        lifecycle_.reject_current();
        captured_ = {};
        active_call_ = nullptr;
    }

    void on_event(BaresipCallbackLifecycle::CapturedSession epoch,
                  enum bevent_ev type, struct bevent* event) {
        if (epoch.fence_generation != captured_.fence_generation ||
            epoch.mailbox_session != captured_.mailbox_session || !active())
            return;
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
            if (!lifecycle_.registration(epoch, BaresipEventKind::Registering))
                fail_closed();
            return;
        case BEVENT_REGISTER_OK:
            if (!lifecycle_.registration(epoch, BaresipEventKind::RegisterOk))
                fail_closed();
            return;
        case BEVENT_REGISTER_FAIL:
            if (!lifecycle_.registration(epoch, BaresipEventKind::RegisterFail))
                fail_closed();
            return;
        case BEVENT_UNREGISTERING:
            if (!lifecycle_.registration(epoch, BaresipEventKind::Unregistering))
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
            if (!lifecycle_.call_started(epoch, new_key, kind)) {
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
            if (!lifecycle_.call_changed(epoch, native_key_, kind)) {
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
    std::vector<std::unique_ptr<CallbackContext>> contexts_;
    BaresipCallbackLifecycle::CapturedSession captured_{};
    std::uint64_t native_key_ = 0;
    bool registered_ = false;
};
} // namespace axv
