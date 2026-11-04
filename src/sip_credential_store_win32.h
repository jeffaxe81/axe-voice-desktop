#pragma once
// Optional per-Windows-user SIP credential storage. Never call from SDK-free
// builds or UI smoke tests. No password is written to app config, logs or registry.
#include <windows.h>
#include <wincred.h>
#include "sip_account.h"
#include <cstring>
#include <cwchar>
#include <string>

namespace axv {
inline std::wstring sip_credential_target(const SipAccount& account) {
    if (validate_account(account) != AccountError::None) return {};
    // Length-prefixing keeps distinct account names unambiguous. Transport
    // and port are part of the identity: a password is never offered to a
    // different SIP account just because its user name matches.
    return L"AXEVoiceDesktop/NativeSIP/" +
        std::to_wstring(account.server.size()) + L":" + account.server + L"/" +
        std::to_wstring(account.username.size()) + L":" + account.username + L"/" +
        std::to_wstring(account.port) + (account.use_tls ? L"/tls" : L"/udp");
}

// The caller owns the UTF-16 buffer and must securely wipe it on every exit.
// CredWriteW stores Generic credentials under the current Windows user's vault.
inline bool save_sip_password(const SipAccount& account,
                              const wchar_t* secret, std::size_t chars) {
    if (!secret || chars == 0 || chars > 256) return false;
    const auto target = sip_credential_target(account);
    if (target.empty()) return false;
    for (std::size_t i = 0; i < chars; ++i)
        if (secret[i] == L'\0') return false;
    CREDENTIALW entry{};
    entry.Type = CRED_TYPE_GENERIC;
    entry.TargetName = const_cast<LPWSTR>(target.c_str());
    entry.CredentialBlobSize = static_cast<DWORD>(chars * sizeof(wchar_t));
    entry.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(secret));
    entry.Persist = CRED_PERSIST_LOCAL_MACHINE;
    entry.UserName = const_cast<LPWSTR>(account.username.c_str());
    return CredWriteW(&entry, 0) != FALSE;
}

inline bool load_sip_password(const SipAccount& account, wchar_t (&out)[257]) {
    SecureZeroMemory(out, sizeof(out));
    const auto target = sip_credential_target(account);
    if (target.empty()) return false;
    PCREDENTIALW entry = nullptr;
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &entry) || !entry)
        return false;
    bool valid = false;
    const DWORD bytes = entry->CredentialBlobSize;
    if (entry->CredentialBlob && bytes >= sizeof(wchar_t) &&
        bytes <= 256 * sizeof(wchar_t) && bytes % sizeof(wchar_t) == 0) {
        const auto chars = bytes / sizeof(wchar_t);
        std::memcpy(out, entry->CredentialBlob, bytes);
        out[chars] = L'\0';
        valid = std::wmemchr(out, L'\0', chars) == nullptr;
        if (!valid) SecureZeroMemory(out, sizeof(out));
    }
    if (entry->CredentialBlob && bytes)
        SecureZeroMemory(entry->CredentialBlob, bytes);
    CredFree(entry);
    return valid;
}

// Unchecking "Lembrar senha" deletes only the currently selected account.
// Missing records count as successfully forgotten.
inline bool forget_sip_password(const SipAccount& account) {
    const auto target = sip_credential_target(account);
    if (target.empty()) return false;
    if (CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0)) return true;
    return GetLastError() == ERROR_NOT_FOUND;
}
} // namespace axv
