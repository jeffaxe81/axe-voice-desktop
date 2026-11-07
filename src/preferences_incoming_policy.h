#pragma once
#include "sip_runtime_observation.h"

namespace axv {
// True only while the exact incoming call shown to the operator remains
// the current, verified call on the same native SIP session.
// Presentation alone never authorizes SIP commands; native engine checks again.
inline bool preferences_incoming_controls_available(
    const SipRuntimeObservation& current,
    const SipRuntimeObservation& displayed) noexcept {
    return current.call_id != 0 && current.sip == SipStatus::Ringing &&
           displayed.sip == SipStatus::Ringing &&
           sip_proof_is_current(current,current.session) &&
           sip_proof_is_current(displayed,current.session) &&
           displayed.session == current.session &&
           displayed.call_id == current.call_id;
}
} // namespace axv
