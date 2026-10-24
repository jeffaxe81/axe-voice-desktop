#pragma once
#include "dial_authorization.h"
#include <cstdint>
#include <string>

namespace axv {

// A snapshot-level authorization for FUTURE in-process call controls.
// Re-check the captured session and call token inside the SIP worker immediately
// before executing any native API call. This pure policy NEVER sends commands.
enum class CallCommandDenied {
    None, UnverifiedSession, EngineUnavailable, InvalidDestination,
    UnsupportedState, StaleCall, EngineStateMismatch
};
struct CallCommandPermit {
    CallCommandDenied reason = CallCommandDenied::UnverifiedSession;
    std::wstring destination;
    bool allowed() const { return reason == CallCommandDenied::None; }
};

inline CallCommandPermit authorize_call_command(
        const SipRuntimeObservation& state, std::uint64_t active_session,
        const EngineSelection& selected, CallAction action,
        std::uint64_t expected_call_id, const std::wstring& destination = {}) {
    if (action == CallAction::Dial) {
        // A dial never targets a prior/active call, even if SIP status somehow
        // becomes inconsistent with the tracked call identity.
        if (expected_call_id != 0 || state.call_id != 0)
            return {CallCommandDenied::StaleCall, {}};
        const auto dial = authorize_dial(state, active_session, selected, destination);
        if (dial.allowed()) return {CallCommandDenied::None, dial.normalized_destination};
        if (dial.reason == DialDenied::EmptyDestination ||
            dial.reason == DialDenied::InvalidDestination)
            return {CallCommandDenied::InvalidDestination, {}};
        if (dial.reason == DialDenied::UnverifiedSession)
            return {CallCommandDenied::UnverifiedSession, {}};
        if (dial.reason == DialDenied::EngineUnavailable)
            return {CallCommandDenied::EngineUnavailable, {}};
        return {CallCommandDenied::EngineStateMismatch, {}};
    }
    if (action != CallAction::Answer && action != CallAction::Hangup)
        return {CallCommandDenied::UnsupportedState, {}};
    if (!sip_proof_is_current(state, active_session))
        return {CallCommandDenied::UnverifiedSession, {}};
    if (selected.kind != EngineKind::Baresip || !selected.engine)
        return {CallCommandDenied::EngineUnavailable, {}};
    if (!may_use_call_control(state, active_session, action))
        return {CallCommandDenied::UnsupportedState, {}};
    if (!expected_call_id || !state.call_id ||
        state.call_id != expected_call_id || state.last_call_id != expected_call_id)
        return {CallCommandDenied::StaleCall, {}};
    if (selected.engine->status() != state.sip)
        return {CallCommandDenied::EngineStateMismatch, {}};
    return {CallCommandDenied::None, {}};
}

inline const wchar_t* call_command_denial_text(CallCommandDenied denied) {
    switch (denied) {
    case CallCommandDenied::None: return L"";
    case CallCommandDenied::UnverifiedSession:
        return L"AXV-SIP-003: sessao SIP nao confirmada.";
    case CallCommandDenied::EngineUnavailable:
        return L"AXV-SIP-001: motor nativo indisponivel.";
    case CallCommandDenied::InvalidDestination:
        return L"AXV-SIP-002: corrija o destino da chamada.";
    case CallCommandDenied::UnsupportedState:
        return L"AXV-SIP-004: acao nao permitida no estado atual.";
    case CallCommandDenied::StaleCall:
        return L"AXV-SIP-006: chamada encerrada ou identificador antigo.";
    case CallCommandDenied::EngineStateMismatch:
        return L"AXV-SIP-007: estado do motor diverge da sessao.";
    }
    return L"AXV-SIP-005: acao indisponivel.";
}
} // namespace axv
