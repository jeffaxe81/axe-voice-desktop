#pragma once
#include "sip_runtime_observation.h"
#include <cstdint>

namespace axv {
// Presentation only. Every SIP command is still reauthorized by the native
// handler using its displayed call identity, never by a visibility flag.
enum class EssentialCallView { Idle, Incoming, Outgoing, Connected };

struct EssentialCallActions {
    EssentialCallView view = EssentialCallView::Idle;
    bool dial = false;
    bool answer = false;
    bool hangup = false;
    bool secondary = false;
};

inline EssentialCallActions essential_call_actions(
    const SipRuntimeObservation& observed,
    std::uint64_t active_session,
    bool dial_authorized) noexcept {
    if (!sip_proof_is_current(observed,active_session))
        return {};
    // A supposedly registered account with an active call token is
    // inconsistent; do not accidentally enable another outgoing call.
    if (observed.sip == SipStatus::Registered)
        return {EssentialCallView::Idle,observed.call_id==0 && dial_authorized,
                false,false,false};
    // Never display actionable call controls without the native call token.
    if (!observed.call_id)
        return {};
    switch(observed.sip) {
    case SipStatus::Ringing:
        return {EssentialCallView::Incoming,false,true,true,false};
    case SipStatus::Calling:
        return {EssentialCallView::Outgoing,false,false,true,false};
    case SipStatus::InCall:
        return {EssentialCallView::Connected,false,false,true,true};
    default:
        return {};
    }
}
} // namespace axv
