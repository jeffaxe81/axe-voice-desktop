#pragma once
#include "sip_runtime_observation.h"
#include <cstdint>

namespace axv {

// Decisions for a future incoming-call ringtone player. This class does not
// produce sound and must not be used as evidence of SIP registration.
// Its input MUST be the UI-thread projection of a VERIFIED native SDK source;
// process uptime, log text and manually triggered audio tests do not qualify.
enum class IncomingRingAction { None, Start, Stop };

class IncomingRingtoneGate final {
public:
    IncomingRingAction observe(const SipRuntimeObservation& state,
                               std::uint64_t verified_session,
                               bool endpoint_available) {
        const bool eligible = endpoint_available &&
            sip_proof_is_current(state, verified_session) &&
            state.sip == SipStatus::Ringing &&
            state.call_id != 0 &&
            state.call_id == state.last_call_id;
        if (active_) {
            // Never bridge an old ringtone into another call or account.
            if (!eligible || session_ != verified_session ||
                call_id_ != state.call_id) {
                active_ = false;
                session_ = call_id_ = 0;
                return IncomingRingAction::Stop;
            }
            return IncomingRingAction::None;
        }
        if (!eligible) return IncomingRingAction::None;
        session_ = verified_session;
        call_id_ = state.call_id;
        active_ = true;
        return IncomingRingAction::Start;
    }

    IncomingRingAction revoke() {
        if (!active_) return IncomingRingAction::None;
        active_ = false;
        session_ = call_id_ = 0;
        return IncomingRingAction::Stop;
    }

    bool active() const { return active_; }
    // Opaque identifiers are internal correlation tokens, never rendered or logged.
private:
    std::uint64_t session_ = 0;
    std::uint64_t call_id_ = 0;
    bool active_ = false;
};
} // namespace axv
