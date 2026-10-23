#pragma once
#include "call_command_authorization.h"
#include <cstdint>
#include <string>

namespace axv {

// Request IDs come from a trusted SIP worker ingress, never from untrusted
// network packets. Each SDK command must be executed on the same serialized
// native event loop that owns the matching call object and live SIP snapshot.
struct SipCommandRequest {
    std::uint64_t session = 0;
    std::uint64_t sequence = 0;
    CallAction action = CallAction::Dial;
    std::uint64_t call_id = 0;  // opaque, never a native pointer
    std::wstring destination;
};

class SipNativeCommandPort {
public:
    virtual ~SipNativeCommandPort() = default;
    virtual bool originate(const std::wstring& normalized_destination) = 0;
    // Implementations MUST resolve the token against the current native-call
    // ownership map and MUST NOT cast the number into a struct call*.
    virtual bool answer(std::uint64_t current_call_token) = 0;
    virtual bool hangup(std::uint64_t current_call_token) = 0;
};

enum class SipCommandDisposition {
    Submitted, InvalidRequest, StaleSession, DuplicateRequest,
    PendingTransition, DuplicateCallAction, Denied, NativeRejected,
    CommandTimedOut, ClockRollback, Fenced
};
struct SipCommandResult {
    SipCommandDisposition disposition = SipCommandDisposition::InvalidRequest;
    CallCommandDenied denial = CallCommandDenied::None;
    bool submitted() const { return disposition == SipCommandDisposition::Submitted; }
};

// Experimental worker-only state machine. NOT linked into AXEVoice.exe.
// It neither grants VerifiedNativeApi nor implements SIP/media. The owner
// serializes this method with callbacks and with native port execution.
class SipCommandExecutor {
public:
    // 'now_ms' MUST come from a monotonic clock on the serialized SIP owner
    // thread, sampled just before dispatch. No wall-clock timestamps.
    static constexpr std::uint64_t kPendingTimeoutMs = 10000;
    SipCommandResult dispatch(const SipCommandRequest& r,
                              const SipRuntimeObservation& live,
                              std::uint64_t active_session,
                              const EngineSelection& selected,
                              SipNativeCommandPort& native,
                              std::uint64_t now_ms) {
        if (!r.session || !r.sequence || !now_ms)
            return {SipCommandDisposition::InvalidRequest};
        if (r.session < session_) return {SipCommandDisposition::StaleSession};
        if (session_ && r.session == session_ && fenced_)
            return {SipCommandDisposition::Fenced};
        if (r.session != active_session ||
            !sip_proof_is_current(live, active_session)) {
            if (r.session == session_) fenced_ = true;
            return {SipCommandDisposition::StaleSession};
        }
        if (r.session > session_) begin_new_session(r.session);
        if (last_now_ms_ && now_ms < last_now_ms_) {
            // A rollback invalidates deadline calculations. Never authorize a
            // native retry while the monotonic time source is untrustworthy.
            fenced_ = true;
            return {SipCommandDisposition::ClockRollback};
        }
        last_now_ms_ = now_ms;
        if (r.sequence <= last_sequence_)
            return {SipCommandDisposition::DuplicateRequest};
        // Consume even denied request IDs. A previously denied command cannot
        // be replayed merely because state/registration changes later.
        last_sequence_ = r.sequence;
        if (pending_) {
            if (live.sip == pending_status_ &&
                live.call_id == pending_call_ &&
                live.last_call_id == pending_last_call_) {
                if (now_ms - pending_started_ms_ >= kPendingTimeoutMs) {
                    // A native success without an observed state change can
                    // conceal a lost callback. Require a new SDK generation.
                    fenced_ = true;
                    return {SipCommandDisposition::CommandTimedOut};
                }
                return {SipCommandDisposition::PendingTransition};
            }
            pending_ = false;
        }
        const auto permission = authorize_call_command(
            live,active_session,selected,r.action,r.call_id,r.destination);
        if (!permission.allowed())
            return {SipCommandDisposition::Denied,permission.reason};
        switch (r.action) {
        case CallAction::Dial:
            if (sent_dial_ && live.last_call_id <= last_dial_call_)
                return {SipCommandDisposition::DuplicateCallAction};
            break;
        case CallAction::Answer:
            if (r.call_id <= last_answer_id_)
                return {SipCommandDisposition::DuplicateCallAction};
            break;
        case CallAction::Hangup:
            if (r.call_id <= last_hangup_id_)
                return {SipCommandDisposition::DuplicateCallAction};
            break;
        }
        bool accepted = false;
        try {
            switch (r.action) {
            case CallAction::Dial: accepted = native.originate(permission.destination); break;
            case CallAction::Answer: accepted = native.answer(r.call_id); break;
            case CallAction::Hangup: accepted = native.hangup(r.call_id); break;
            }
        } catch (...) {
            accepted = false; // do not expose native errors or SIP secrets
        }
        if (!accepted) {
            // Native side effects may have happened before an error. Never
            // retry on the same session without an explicit new generation.
            fenced_ = true;
            return {SipCommandDisposition::NativeRejected};
        }
        switch (r.action) {
        case CallAction::Dial:
            sent_dial_ = true; last_dial_call_ = live.last_call_id; break;
        case CallAction::Answer: last_answer_id_ = r.call_id; break;
        case CallAction::Hangup: last_hangup_id_ = r.call_id; break;
        }
        pending_ = true;
        pending_started_ms_ = now_ms;
        pending_status_ = live.sip;
        pending_call_ = live.call_id;
        pending_last_call_ = live.last_call_id;
        return {SipCommandDisposition::Submitted};
    }

private:
    void begin_new_session(std::uint64_t id) {
        session_ = id;
        last_sequence_ = 0;
        last_answer_id_ = 0;
        last_hangup_id_ = 0;
        last_dial_call_ = 0;
        pending_call_ = 0;
        pending_last_call_ = 0;
        pending_status_ = SipStatus::Unavailable;
        pending_started_ms_ = 0;
        last_now_ms_ = 0;
        pending_ = false;
        sent_dial_ = false;
        fenced_ = false;
    }
    std::uint64_t session_ = 0, last_sequence_ = 0;
    std::uint64_t last_answer_id_ = 0, last_hangup_id_ = 0, last_dial_call_ = 0;
    std::uint64_t pending_call_ = 0, pending_last_call_ = 0;
    std::uint64_t pending_started_ms_ = 0, last_now_ms_ = 0;
    SipStatus pending_status_ = SipStatus::Unavailable;
    bool pending_ = false, sent_dial_ = false, fenced_ = false;
};
} // namespace axv
