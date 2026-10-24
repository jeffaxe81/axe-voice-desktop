#pragma once
// Pure offline preflight for a FUTURE TLS-only native SIP identity.
// No network, SIP SDK, credential read or registration happens here.
#include "account_form_feedback.h"
#include <string>
#include <cstddef>

namespace axv {
enum class SipNativePreflightStatus {
    Incomplete, InvalidFields, UnsupportedNativeIdentity,
    TlsRequired, EligibleForNativePreparation
};
struct SipNativePreflight {
    SipNativePreflightStatus status = SipNativePreflightStatus::Incomplete;
    unsigned short port = 0;
    bool eligible_for_preparation = false;
    bool registration_verified = false;
    bool media_verified = false;
};

// The isolated Baresip UA factory accepts ASCII-only tokens and DNS names.
// Validate the same *public shape* without importing the audited SDK into GUI.
inline bool safe_native_identity(const std::wstring& server,
                                 const std::wstring& username) noexcept {
    if (server.empty() || server.size() > 253 ||
        username.empty() || username.size() > 64) return false;
    for (wchar_t c : username) {
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'_' || c == L'-' || c == L'.'))
            return false;
    }
    std::size_t label_length = 0;
    for (std::size_t i = 0; i < server.size(); ++i) {
        const wchar_t c = server[i];
        if (c == L'.') {
            if (!label_length || label_length > 63 || server[i - 1] == L'-')
                return false;
            label_length = 0;
            continue;
        }
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'-')) return false;
        if (!label_length && c == L'-') return false;
        if (++label_length > 63) return false;
    }
    return label_length != 0 && server.back() != L'-';
}

inline SipNativePreflight evaluate_native_sip_preflight(
    const std::wstring& server, const std::wstring& username,
    const std::wstring& port_text, bool tls_selected) {
    const auto fields = evaluate_account_form(server, username, port_text);
    if (fields.kind == AccountFeedbackKind::Incomplete)
        return {SipNativePreflightStatus::Incomplete};
    if (!fields.valid())
        return {SipNativePreflightStatus::InvalidFields};
    if (!safe_native_identity(server, username))
        return {SipNativePreflightStatus::UnsupportedNativeIdentity};
    if (!tls_selected)
        return {SipNativePreflightStatus::TlsRequired, fields.port};
    return {SipNativePreflightStatus::EligibleForNativePreparation,
            fields.port, true, false, false};
}

inline const wchar_t* native_preflight_label(SipNativePreflightStatus status) {
    switch (status) {
    case SipNativePreflightStatus::Incomplete:
        return L"Campos de conta SIP incompletos";
    case SipNativePreflightStatus::InvalidFields:
        return L"Dados de conta invalidos (somente validacao local)";
    case SipNativePreflightStatus::UnsupportedNativeIdentity:
        return L"Identidade fora do formato DNS/ASCII aceito pelo motor nativo";
    case SipNativePreflightStatus::TlsRequired:
        return L"Sinalizacao TLS obrigatoria para integracao nativa";
    case SipNativePreflightStatus::EligibleForNativePreparation:
        return L"Identidade elegivel a preparacao local; registro NAO realizado";
    }
    return L"Preflight SIP indisponivel";
}
} // namespace axv
