#pragma once
#include "sip_runtime_observation.h"
#include <cstdint>

namespace axv {
// These represent Baresip's native enum bevent_ev; not parsed from console text.
// The actual Baresip C API is NOT linked in this prototype.
enum class BaresipEventKind {
    Registering, RegisterOk, RegisterFail, Unregistering,
    CallIncoming, CallOutgoing, CallEstablished, CallClosed,
    TransportLost
};
struct BaresipNativeEvent {
    BaresipEventKind kind;
    std::uint64_t session; // Assigned by the future trusted in-process adapter.
};
enum class EventTransition { Applied, StaleSession, UnverifiedSource, InvalidOrder };

// Reducer must execute on the UI thread (worker callbacks only enqueue).
// Trust must be established OUTSIDE this reducer by the future native adapter.
inline EventTransition apply_baresip_event(SipRuntimeObservation& state,
                                          std::uint64_t active_session,
                                          const BaresipNativeEvent& event) {
    if (is_stale_session(event.session, active_session) ||
        state.session != active_session)
        return EventTransition::StaleSession;
    if (!sip_proof_is_current(state, active_session))
        return EventTransition::UnverifiedSource;

    const auto current = state.sip;
    SipStatus next = current;
    switch (event.kind) {
    case BaresipEventKind::Registering:
        if (current != SipStatus::Unavailable && current != SipStatus::Disconnected &&
            current != SipStatus::Failed && current != SipStatus::Registered)
            return EventTransition::InvalidOrder;
        next = SipStatus::Registering;
        break;
    case BaresipEventKind::RegisterOk:
        if (current != SipStatus::Registering) return EventTransition::InvalidOrder;
        next = SipStatus::Registered;
        break;
    case BaresipEventKind::RegisterFail:
        if (current != SipStatus::Registering) return EventTransition::InvalidOrder;
        next = SipStatus::Failed;
        break;
    case BaresipEventKind::Unregistering:
        if (current != SipStatus::Registering && current != SipStatus::Registered)
            return EventTransition::InvalidOrder;
        next = SipStatus::Disconnected;
        break;
    case BaresipEventKind::CallIncoming:
        if (current != SipStatus::Registered) return EventTransition::InvalidOrder;
        next = SipStatus::Ringing;
        break;
    case BaresipEventKind::CallOutgoing:
        if (current != SipStatus::Registered) return EventTransition::InvalidOrder;
        next = SipStatus::Calling;
        break;
    case BaresipEventKind::CallEstablished:
        if (current != SipStatus::Calling && current != SipStatus::Ringing)
            return EventTransition::InvalidOrder;
        next = SipStatus::InCall;
        break;
    case BaresipEventKind::CallClosed:
        if (current != SipStatus::Calling && current != SipStatus::Ringing &&
            current != SipStatus::InCall) return EventTransition::InvalidOrder;
        next = SipStatus::Registered;
        break;
    case BaresipEventKind::TransportLost:
        // The engine is no longer safely usable; repeated disconnection is idempotent.
        next = SipStatus::Disconnected;
        break;
    }
    state.sip = next;
    return EventTransition::Applied;
}
} // namespace axv
