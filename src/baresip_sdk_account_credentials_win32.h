#pragma once
// EXPERIMENTAL / AUDITED-SDK-BUILD ONLY. Not linked into AXEVoice.exe.
// Uses Baresip account API rather than placing credentials in an AOR, file,
// command line, queue, log, or UI. The SDK retains a heap copy until cleared.
#ifndef _WIN32
#error "Windows Credential Manager is required for the SDK credential bridge"
#endif
#include "baresip_native_account_lifecycle.h"
#include "sip_vault_status_win32.h"
#include <baresip.h>
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace axv {
constexpr std::size_t kSdkSecretMax = 256;

// Generic Windows credentials may be entered as ASCII bytes or as
// UTF-16LE encoding of printable ASCII. Anything ambiguous/non-printable
// is refused; no automatic locale conversion or truncation is performed.
// This deliberately limits compatible passwords, not the user's account.
inline bool decode_sip_secret_ascii(
    const std::uint8_t* blob, std::size_t length,
    std::array<char,kSdkSecretMax+1>& output) noexcept {
    // Credential Manager can supply ASCII or UTF-16LE bytes. Accept ONE
    // optional trailing terminator in either format, never embedded NUL.
    // No replacement characters, lossy conversion or silent truncation.
    if (!blob || !length || length > (kSdkSecretMax + 1) * 2)
        return false;
    bool utf16le = length % 2 == 0;
    if (utf16le) {
        for (std::size_t i=1; i<length; i+=2) {
            if (blob[i] != 0) { utf16le = false; break; }
        }
    }
    std::size_t n = utf16le ? length / 2 : length;
    if (utf16le) {
        if (n && blob[(n-1)*2] == 0) --n;
    } else if (n && blob[n-1] == 0) {
        --n;
    }
    if (!n || n > kSdkSecretMax) return false;
    for (std::size_t i=0; i<n; ++i) {
        const unsigned char ch = blob[utf16le ? i*2 : i];
        if (ch < 0x20 || ch > 0x7e) return false;
        output[i] = static_cast<char>(ch);
    }
    output[n] = '\0';
    return true;
}

inline void clear_sdk_account_password(struct ua* selected) noexcept {
    if (!selected) return;
    struct account* account = ua_account(selected);
    if (!account) return;
    // Reviewed Baresip account_set_auth_pass uses str_dup / mem_deref.
    // With an AUDITED SDK version, the getter addresses the account-owned
    // writable copy. Wipe BEFORE the SDK frees that allocation.
    const char* stored = account_auth_pass(account);
    if (stored) {
        const std::size_t size = std::strlen(stored);
        SecureZeroMemory(const_cast<char*>(stored), size);
        (void)account_set_auth_pass(account, nullptr);
    }
}

inline std::optional<NativeUaAccountCredentialHooks>
make_baresip_account_wincred_hooks(const NativeUaIdentity& identity) {
    if (!native_ua_safe_aor(identity)) return std::nullopt;
    const std::wstring user(identity.user.begin(), identity.user.end());
    const std::wstring domain(identity.domain.begin(), identity.domain.end());
    const auto alias = sip_vault_alias(user, domain);
    if (alias.empty()) return std::nullopt;
    NativeUaAccountCredentialHooks hooks;
    hooks.install = [alias](struct ua* selected) {
        if (!selected) return false;
        struct account* account = ua_account(selected);
        if (!account) return false;
        const auto result = consume_sip_credential_on_owner(
            alias, std::this_thread::get_id(),
            [account](const std::uint8_t* secret, std::size_t length) {
                std::array<char,kSdkSecretMax+1> scratch{};
                struct Scrubber {
                    std::array<char,kSdkSecretMax+1>& value;
                    ~Scrubber() { SecureZeroMemory(value.data(), value.size()); }
                } wipe{scratch};
                if (!decode_sip_secret_ascii(secret, length, scratch))
                    return false;
                // Setter duplicates into the SDK-owned account on its thread.
                // Caller-owned scratch is wiped even on failure.
                return account_set_auth_pass(account, scratch.data()) == 0;
            });
        return result == SipCredentialResult::Consumed;
    };
    hooks.clear = [](struct ua* selected) {
        clear_sdk_account_password(selected);
    };
    return hooks;
}
} // namespace axv
