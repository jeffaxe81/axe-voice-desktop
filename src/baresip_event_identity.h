#pragma once
#include "baresip_event_reducer.h"
#include <cstdint>

namespace axv {
// One selected account and one active call. The future native adapter must
// mint opaque, generation-scoped, increasing tokens (never reuse call pointers).
struct BoundBaresipState {
    SipRuntimeObservation observation{};
    std::uint64_t expected_account_token = 0;
    std::uint64_t active_call_token = 0;
    std::uint64_t last_call_token = 0;
};
enum class IdentityResult {
    Applied, StaleSession, UnverifiedSource, WrongAccount, MissingCall,
    ConcurrentCall, WrongCall, ReplayedCall, UnexpectedCall, InvalidOrder
};
inline IdentityResult apply_identified_event(BoundBaresipState& bound,
                                            std::uint64_t active_session,
                                            const BaresipNativeEvent& event) {
    if (is_stale_session(event.session, active_session) ||
        bound.observation.session != active_session) return IdentityResult::StaleSession;
    if (!sip_proof_is_current(bound.observation, active_session))
        return IdentityResult::UnverifiedSource;
    if (!bound.expected_account_token || event.account_token != bound.expected_account_token)
        return IdentityResult::WrongAccount;
    const bool opening = event.kind == BaresipEventKind::CallIncoming ||
                         event.kind == BaresipEventKind::CallOutgoing;
    const bool continuing = event.kind == BaresipEventKind::CallEstablished ||
                            event.kind == BaresipEventKind::CallClosed;
    if (opening) {
        if (!event.call_token) return IdentityResult::MissingCall;
        if (bound.active_call_token) return IdentityResult::ConcurrentCall;
        if (event.call_token <= bound.last_call_token) return IdentityResult::ReplayedCall;
    } else if (continuing) {
        if (!event.call_token) return IdentityResult::MissingCall;
        if (event.call_token != bound.active_call_token) return IdentityResult::WrongCall;
    } else if (event.call_token) return IdentityResult::UnexpectedCall;
    auto identified = event;
    identified.call_id = event.call_token;
    const auto result = apply_baresip_event(bound.observation, active_session, identified);
    if (result != EventTransition::Applied) return IdentityResult::InvalidOrder;
    if (opening) {
        bound.active_call_token = event.call_token;
        bound.last_call_token = event.call_token;
    } else if (event.kind == BaresipEventKind::CallClosed ||
               event.kind == BaresipEventKind::TransportLost) {
        bound.active_call_token = 0;
    }
    return IdentityResult::Applied;
}
inline void invalidate_identity(BoundBaresipState& bound) {
    bound = {}; // Drops native provenance, account and active call on shutdown.
}
} // namespace axv
