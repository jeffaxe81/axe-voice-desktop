#pragma once
#include "baresip_event_mailbox.h"
#include <cstdint>
#include <limits>
#include <mutex>

namespace axv {

// Pre-admission contract ONLY. Not a Baresip C API implementation, and
// NOT a source of SipEvidence::VerifiedNativeApi.
// Opaque identities are validated by a FUTURE audited in-process bevent_h
// adapter. Do not use console output, URIs, SIP passwords or user input.
enum class NativeSignal {
    Registering, RegisterOk, RegisterFail, Unregistering,
    CallIncoming, CallOutgoing, CallEstablished, CallClosed,
    TransportLost, Unrelated
};
struct NativeCallback {
    NativeSignal signal;
    std::uint64_t session;
    std::uintptr_t account;
    std::uintptr_t call;
};
enum class NativeIngressResult { Queued, Ignored, Rejected, Revoked };
enum class NativeIngressFault {
    None, SessionMismatch, AccountMismatch, CallMismatch,
    Malformed, Unrecognized, QueueRejected
};

class BaresipNativeIngress {
public:
    explicit BaresipNativeIngress(BaresipEventMailbox& queue) : queue_(queue) {}
    BaresipNativeIngress(const BaresipNativeIngress&) = delete;
    BaresipNativeIngress& operator=(const BaresipNativeIngress&) = delete;

    // The future trusted adapter, not the UI, will call this AFTER the
    // process/UA has been admitted and its callback source checked.
    std::uint64_t bind_validated_account(std::uintptr_t account) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!account || session_ || fault_ != NativeIngressFault::None) return 0;
        const auto next = queue_.begin_session();
        if (!next) {
            fault_ = NativeIngressFault::QueueRejected;
            return 0;
        }
        account_ = account;
        session_ = next;
        call_ = 0;
        call_token_ = 0;
        last_call_token_ = 0;
        return next;
    }

    NativeIngressResult accept(const NativeCallback& cb) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!session_ || fault_ != NativeIngressFault::None)
            return NativeIngressResult::Rejected;
        if (cb.session != session_)
            return revoke(NativeIngressFault::SessionMismatch);
        if (!cb.account || cb.account != account_)
            return revoke(NativeIngressFault::AccountMismatch);

        BaresipEventKind event = BaresipEventKind::Registering;
        bool call_signal = false;
        bool starts_call = false;
        bool closes_call = false;
        switch (cb.signal) {
        case NativeSignal::Registering: event = BaresipEventKind::Registering; break;
        case NativeSignal::RegisterOk: event = BaresipEventKind::RegisterOk; break;
        case NativeSignal::RegisterFail: event = BaresipEventKind::RegisterFail; break;
        case NativeSignal::Unregistering: event = BaresipEventKind::Unregistering; break;
        case NativeSignal::CallIncoming:
            event = BaresipEventKind::CallIncoming;
            call_signal = starts_call = true; break;
        case NativeSignal::CallOutgoing:
            event = BaresipEventKind::CallOutgoing;
            call_signal = starts_call = true; break;
        case NativeSignal::CallEstablished:
            event = BaresipEventKind::CallEstablished;
            call_signal = true; break;
        case NativeSignal::CallClosed:
            event = BaresipEventKind::CallClosed;
            call_signal = closes_call = true; break;
        case NativeSignal::TransportLost: event = BaresipEventKind::TransportLost; break;
        case NativeSignal::Unrelated:
            return cb.call == 0 ? NativeIngressResult::Ignored
                                : revoke(NativeIngressFault::Malformed);
        default: return revoke(NativeIngressFault::Unrecognized);
        }

        if (!call_signal && cb.call)
            return revoke(NativeIngressFault::Malformed);
        if (call_signal) {
            if (!cb.call) return revoke(NativeIngressFault::CallMismatch);
            if (starts_call && !call_) {
                // Reserve identity only after enqueue succeeds.
            } else if (!call_ || cb.call != call_) {
                return revoke(NativeIngressFault::CallMismatch);
            }
        }
        // Baresip call pointers are NOT durable sequence IDs. Translate the
        // validated active-call identity into a monotonic per-session token.
        std::uint64_t event_call_token = 0;
        if (call_signal) {
            if (starts_call && !call_) {
                if (last_call_token_ == (std::numeric_limits<std::uint64_t>::max)())
                    return revoke(NativeIngressFault::CallMismatch);
                event_call_token = last_call_token_ + 1;
            } else {
                event_call_token = call_token_;
            }
        }
        if (!queue_.enqueue({event, session_, event_call_token}))
            return revoke(NativeIngressFault::QueueRejected);
        if (starts_call && !call_) {
            call_ = cb.call;
            call_token_ = last_call_token_ = event_call_token;
        }
        if (closes_call || cb.signal == NativeSignal::TransportLost) {
            call_ = 0;
            call_token_ = 0;
        }
        return NativeIngressResult::Queued;
    }

    // Only after callback unsubscription and worker/UA teardown. The next
    // binding gets a distinct monotonic queue session.
    void reset_after_teardown() {
        std::lock_guard<std::mutex> lock(mu_);
        queue_.invalidate_session();
        session_ = 0;
        account_ = call_ = 0;
        call_token_ = last_call_token_ = 0;
        fault_ = NativeIngressFault::None;
    }
    std::uint64_t session() const {
        std::lock_guard<std::mutex> lock(mu_);
        return session_;
    }
    NativeIngressFault fault() const {
        std::lock_guard<std::mutex> lock(mu_);
        return fault_;
    }
private:
    NativeIngressResult revoke(NativeIngressFault why) {
        queue_.invalidate_session();  // revoke ALL already queued SIP evidence
        session_ = 0;
        account_ = call_ = 0;
        call_token_ = last_call_token_ = 0;
        fault_ = why;
        return NativeIngressResult::Revoked;
    }
    BaresipEventMailbox& queue_;
    mutable std::mutex mu_;
    std::uint64_t session_ = 0;
    std::uintptr_t account_ = 0;
    std::uintptr_t call_ = 0;
    std::uint64_t call_token_ = 0;
    std::uint64_t last_call_token_ = 0;
    NativeIngressFault fault_ = NativeIngressFault::None;
};
} // namespace axv
