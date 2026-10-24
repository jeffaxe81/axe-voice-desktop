#pragma once
// Windows-only, local, metadata-only readiness check. NEVER returns a secret.
// This component is not a SIP authentication provider and authorizes no calls.
#ifndef _WIN32
#error "Windows Credential Manager only"
#endif
#include "sip_native_preflight.h"
#include "sip_credential_lease_win32.h"
#include <windows.h>
#include <wincred.h>
#include <string>
#include <cstddef>

namespace axv {

enum class SipVaultStatus {
    AccountInvalid, AliasTooLong, Missing, AccessDenied,
    InspectionFailed, InvalidStoredRecord, PresentUnverified
};

// Length-prefix the username so two pairs of identity parts cannot collide.
// An alias is not the password or proof of SIP authentication. The username
// and DNS host are used only to locate the Windows generic credential.
inline std::wstring sip_vault_alias(const std::wstring& user,
                                   const std::wstring& dns_host) {
    if (!safe_native_identity(dns_host, user)) return {};
    std::wstring result = L"AXEVoice/SIP/v1.";
    result += std::to_wstring(user.size()) + L"." + user + L"." + dns_host;
    if (!sip_credential_target_safe(result)) return {};
    return result;
}

// Credential Manager can return a full secret blob even for existence checks.
// NEVER inspect or copy it. Wipe and free the Win32 structure immediately.
inline SipVaultStatus inspect_sip_vault_presence(
    const std::wstring& dns_host, const std::wstring& user) {
    if (!safe_native_identity(dns_host, user))
        return SipVaultStatus::AccountInvalid;
    const auto alias = sip_vault_alias(user, dns_host);
    if (alias.empty()) return SipVaultStatus::AliasTooLong;
    PCREDENTIALW record = nullptr;
    if (!CredReadW(alias.c_str(), CRED_TYPE_GENERIC, 0, &record)) {
        // A permission failure or OS error is not a "missing password".
        const DWORD error = GetLastError();
        return error == ERROR_NOT_FOUND ? SipVaultStatus::Missing :
               error == ERROR_ACCESS_DENIED ? SipVaultStatus::AccessDenied :
                                              SipVaultStatus::InspectionFailed;
    }
    if (!record) return SipVaultStatus::InspectionFailed;
    // Do not trust Win32 record metadata beyond this narrow alias/size check.
    // Even a present secret may be wrong or unauthorized for this registrar.
    const bool meaningful = record->Type == CRED_TYPE_GENERIC &&
        record->TargetName && alias == record->TargetName &&
        record->CredentialBlob &&
        record->CredentialBlobSize > 0 && record->CredentialBlobSize <= 2048;
    if (record->CredentialBlob && record->CredentialBlobSize)
        SecureZeroMemory(record->CredentialBlob, record->CredentialBlobSize);
    CredFree(record);
    return meaningful ? SipVaultStatus::PresentUnverified :
                        SipVaultStatus::InvalidStoredRecord;
}

enum class SipVaultRemoval {
    InvalidIdentity, AliasTooLong, Removed, AlreadyMissing, AccessDenied, Failed
};

// Explicit user action only. No automatic credential deletion during a failed
// SIP registration, process restart, probe, or application shutdown.
inline SipVaultRemoval remove_sip_vault_credential(
    const std::wstring& dns_host, const std::wstring& user) {
    if (!safe_native_identity(dns_host, user))
        return SipVaultRemoval::InvalidIdentity;
    const auto alias = sip_vault_alias(user, dns_host);
    if (alias.empty()) return SipVaultRemoval::AliasTooLong;
    if (CredDeleteW(alias.c_str(), CRED_TYPE_GENERIC, 0))
        return SipVaultRemoval::Removed;
    const DWORD error = GetLastError();
    return error == ERROR_NOT_FOUND ? SipVaultRemoval::AlreadyMissing :
           error == ERROR_ACCESS_DENIED ? SipVaultRemoval::AccessDenied :
                                          SipVaultRemoval::Failed;
}
inline const wchar_t* sip_vault_removal_label(SipVaultRemoval value) noexcept {
    switch (value) {
    case SipVaultRemoval::InvalidIdentity:
        return L"Informe uma identidade DNS/usuario valida para remover.";
    case SipVaultRemoval::AliasTooLong:
        return L"Identificador do cofre excede o limite permitido.";
    case SipVaultRemoval::Removed:
        return L"Credencial SIP local excluida do cofre do Windows.";
    case SipVaultRemoval::AlreadyMissing:
        return L"Nenhuma credencial SIP correspondente foi encontrada.";
    case SipVaultRemoval::AccessDenied:
        return L"O Windows negou a exclusao da credencial.";
    case SipVaultRemoval::Failed:
        return L"Falha ao excluir credencial; confira permissoes no Windows.";
    }
    return L"Resultado de exclusao do cofre indisponivel.";
}

inline const wchar_t* sip_vault_status_label(SipVaultStatus s) noexcept {
    switch (s) {
    case SipVaultStatus::AccountInvalid: return L"Identidade local invalida";
    case SipVaultStatus::AliasTooLong: return L"Identificador da conta excede limite seguro";
    case SipVaultStatus::Missing: return L"Credencial nao localizada no Windows";
    case SipVaultStatus::AccessDenied: return L"Windows negou acesso ao cofre";
    case SipVaultStatus::InspectionFailed: return L"Falha ao consultar o cofre do Windows";
    case SipVaultStatus::InvalidStoredRecord:
        return L"Registro de credencial sem segredo valido ou tipo incorreto";
    case SipVaultStatus::PresentUnverified:
        return L"Credencial local encontrada (autenticacao NAO confirmada)";
    }
    return L"Estado do cofre indisponivel";
}

} // namespace axv
