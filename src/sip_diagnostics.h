#pragma once
#include <string>
namespace axv {
// No credentials are logged. Only fixed diagnostic identifiers are emitted.
enum class DiagnosticCode { EngineMissing, InvalidAccount, RegistrationPending, TransportError, CallUnavailable };
inline const wchar_t* diagnostic_text(DiagnosticCode code) {
    switch(code) {
    case DiagnosticCode::EngineMissing: return L"AXV-SIP-001: motor SIP não integrado";
    case DiagnosticCode::InvalidAccount: return L"AXV-SIP-002: conta SIP inválida";
    case DiagnosticCode::RegistrationPending: return L"AXV-SIP-003: registro não confirmado";
    case DiagnosticCode::TransportError: return L"AXV-SIP-004: falha de transporte";
    case DiagnosticCode::CallUnavailable: return L"AXV-SIP-005: chamada indisponível";
    }
    return L"AXV-SIP-999: diagnóstico desconhecido";
}
}
