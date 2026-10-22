#pragma once
#include "baresip_event_mailbox.h"
#include <cstdint>
#include <limits>
#include <mutex>

namespace axv {
// Experimental adapter-side transport for callbacks from an audited in-process
// native API. Does NOT prove the caller's provenance or grant VerifiedNativeApi.
// It is intentionally NOT instantiated by the Windows frontend yet.
//
// A future adapter must capture 'session' when registering each native callback,
// and issue a fresh strictly increasing 'native_call_key' per call, retaining the
// same key for that call's subsequent callbacks. No URI, credentials or raw
// pointers are copied into the event mailbox.
class BaresipCallbackIngress {
public:
    explicit BaresipCallbackIngress(BaresipEventMailbox& mailbox)
        : mailbox_(mailbox) {}

    // Identity must come from a future audited native UA callback adapter.
    // An arbitrary caller supplying a value does NOT establish provenance.
    std::uint64_t begin_session(std::uintptr_t validated_account) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!validated_account || session_ != 0) return 0;
        session_ = mailbox_.begin_session();
        account_ = session_ ? validated_account : 0;
        last_native_key_ = 0;
        last_token_ = 0;
        active_native_key_ = 0;
        active_token_ = 0;
        return session_;
    }

    void revoke() {
        std::lock_guard<std::mutex> lock(mutex_);
        revoke_locked();
    }

    bool registration(std::uint64_t callback_session, std::uintptr_t callback_account,
                      BaresipEventKind kind) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!admit_locked(callback_session, callback_account)) return false;
        switch (kind) {
        case BaresipEventKind::Registering:
        case BaresipEventKind::RegisterOk:
        case BaresipEventKind::RegisterFail:
        case BaresipEventKind::Unregistering:
            break;
        case BaresipEventKind::TransportLost:
            active_native_key_ = active_token_ = 0;
            break;
        default:
            revoke_locked();
            return false;
        }
        return enqueue_locked(kind, 0);
    }

    bool call_started(std::uint64_t callback_session, std::uintptr_t callback_account,
                      std::uint64_t native_call_key, BaresipEventKind kind) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!admit_locked(callback_session, callback_account)) return false;
        if ((kind != BaresipEventKind::CallIncoming &&
             kind != BaresipEventKind::CallOutgoing) ||
            native_call_key == 0 || native_call_key <= last_native_key_ ||
            active_native_key_ != 0 ||
            last_token_ == std::numeric_limits<std::uint64_t>::max()) {
            revoke_locked();
            return false;
        }
        const auto token = last_token_ + 1;
        if (!enqueue_locked(kind, token)) return false;
        active_native_key_ = native_call_key;
        active_token_ = token;
        last_native_key_ = native_call_key;
        last_token_ = token;
        return true;
    }

    bool call_changed(std::uint64_t callback_session, std::uintptr_t callback_account,
                      std::uint64_t native_call_key, BaresipEventKind kind) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!admit_locked(callback_session, callback_account)) return false;
        if ((kind != BaresipEventKind::CallEstablished &&
             kind != BaresipEventKind::CallClosed) ||
            native_call_key == 0 || native_call_key != active_native_key_ ||
            active_token_ == 0) {
            revoke_locked();
            return false;
        }
        if (!enqueue_locked(kind, active_token_)) return false;
        if (kind == BaresipEventKind::CallClosed) {
            active_native_key_ = 0;
            active_token_ = 0;
        }
        return true;
    }

    std::uint64_t session() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return session_;
    }

private:
    bool is_current_locked(std::uint64_t callback_session) const {
        return session_ != 0 && callback_session == session_;
    }
    bool admit_locked(std::uint64_t callback_session,
                      std::uintptr_t callback_account) {
        // Late callbacks from a different generation cannot poison the new one.
        if (!is_current_locked(callback_session)) return false;
        if (!callback_account || callback_account != account_) {
            revoke_locked(); // mismatch invalidates the whole queued history
            return false;
        }
        return true;
    }
    bool enqueue_locked(BaresipEventKind kind, std::uint64_t call_token) {
        if (mailbox_.enqueue({kind, session_, call_token})) return true;
        // Includes overflow or external invalidation. Never use partial history.
        revoke_locked();
        return false;
    }
    void revoke_locked() {
        mailbox_.invalidate_session();
        session_ = 0;
        account_ = 0;
        active_native_key_ = active_token_ = 0;
        last_native_key_ = last_token_ = 0;
    }

    BaresipEventMailbox& mailbox_;
    mutable std::mutex mutex_;
    std::uint64_t session_ = 0;
    std::uintptr_t account_ = 0;
    std::uint64_t active_native_key_ = 0;
    std::uint64_t active_token_ = 0;
    std::uint64_t last_native_key_ = 0;
    std::uint64_t last_token_ = 0;
};
} // namespace axv
