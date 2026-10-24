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
    // Zero for registration/transport events. A call must have a nonzero opaque
    // strictly-increasing token, retained for all events of that same call.
    std::uint64_t call_id = 0;
};
enum class EventTransition { Applied, StaleSession, UnverifiedSource, InvalidOrder, StaleCall };

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
    std::uint64_t next_call_id = state.call_id;
    std::uint64_t last_call_id = state.last_call_id;
    switch (event.kind) {
    case BaresipEventKind::Registering:
        if (event.call_id || (current != SipStatus::Unavailable &&
            current != SipStatus::Disconnected && current != SipStatus::Failed &&
            current != SipStatus::Registered))
            return EventTransition::InvalidOrder;
        next = SipStatus::Registering;
        break;
    case BaresipEventKind::RegisterOk:
        if (event.call_id || current != SipStatus::Registering) return EventTransition::InvalidOrder;
        next = SipStatus::Registered;
        break;
    case BaresipEventKind::RegisterFail:
        if (event.call_id || current != SipStatus::Registering) return EventTransition::InvalidOrder;
        next = SipStatus::Failed;
        break;
    case BaresipEventKind::Unregistering:
        if (event.call_id || (current != SipStatus::Registering && current != SipStatus::Registered))
            return EventTransition::InvalidOrder;
        next = SipStatus::Disconnected;
        break;
    case BaresipEventKind::CallIncoming:
    case BaresipEventKind::CallOutgoing:
        if (current != SipStatus::Registered) return EventTransition::InvalidOrder;
        if (!event.call_id || next_call_id || event.call_id <= last_call_id)
            return EventTransition::StaleCall;
        next_call_id = last_call_id = event.call_id;
        next = event.kind == BaresipEventKind::CallIncoming
            ? SipStatus::Ringing : SipStatus::Calling;
        break;
    case BaresipEventKind::CallEstablished:
        if (current != SipStatus::Calling && current != SipStatus::Ringing)
            return EventTransition::InvalidOrder;
        if (!event.call_id || event.call_id != next_call_id)
            return EventTransition::StaleCall;
        next = SipStatus::InCall;
        break;
    case BaresipEventKind::CallClosed:
        if (current != SipStatus::Calling && current != SipStatus::Ringing &&
            current != SipStatus::InCall)
            return EventTransition::InvalidOrder;
        if (!event.call_id || event.call_id != next_call_id)
            return EventTransition::StaleCall;
        next_call_id = 0;
        next = SipStatus::Registered;
        break;
    case BaresipEventKind::TransportLost:
        if (event.call_id) return EventTransition::InvalidOrder;
        next_call_id = 0;
        // The engine is no longer safely usable; repeated disconnection is idempotent.
        next = SipStatus::Disconnected;
        break;
    }
    state.sip = next;
    state.call_id = next_call_id;
    state.last_call_id = last_call_id;
    return EventTransition::Applied;
}
} // namespace axv
