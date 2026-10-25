#pragma once
#include "sip_address.h"
#include "sip_engine_factory.h"
#include "sip_runtime_observation.h"
#include <cstdint>
#include <string>
#include <utility>

namespace axv {
// Single authorization point for both the enabled state of "Chamar" and
// the actual click handler. Never infer SIP proof from a process or text log.
enum class DialDenied {
    None, EmptyDestination, InvalidDestination,
    UnverifiedSession, EngineUnavailable, EngineNotRegistered
};
struct DialAuthorization {
    DialDenied reason = DialDenied::UnverifiedSession;
    std::wstring normalized_destination;
    bool allowed() const { return reason == DialDenied::None; }
};

// Presentation gate only; future Baresip adapter must independently validate
// registration, call ownership and media security before sending SIP requests.
inline DialAuthorization authorize_dial(const SipRuntimeObservation& observed,
                                        std::uint64_t current_session,
                                        const EngineSelection& selected,
                                        const std::wstring& typed_destination) {
    if (typed_destination.empty())
        return {DialDenied::EmptyDestination, {}};
    const AddressResult address = validate_address(typed_destination);
    if (!address)
        return {DialDenied::InvalidDestination, {}};
    if (!may_use_call_control(observed, current_session, CallAction::Dial))
        return {DialDenied::UnverifiedSession, {}};
    if (selected.kind != EngineKind::Baresip || !selected.engine)
        return {DialDenied::EngineUnavailable, {}};
    if (selected.engine->status() != SipStatus::Registered)
        return {DialDenied::EngineNotRegistered, {}};
    return {DialDenied::None, address.normalized};
}

inline const wchar_t* dial_denial_text(DialDenied reason) {
    switch (reason) {
    case DialDenied::None: return L"";
    case DialDenied::EmptyDestination: return L"Informe um número ou SIP URI.";
    case DialDenied::InvalidDestination: return L"Destino SIP inválido.";
    case DialDenied::UnverifiedSession: return L"AXV-SIP-003: registro SIP não confirmado.";
    case DialDenied::EngineUnavailable: return L"AXV-SIP-001: motor SIP não integrado.";
    case DialDenied::EngineNotRegistered: return L"AXV-SIP-003: motor não registrado.";
    }
    return L"AXV-SIP-005: chamada indisponível.";
}
} // namespace axv
