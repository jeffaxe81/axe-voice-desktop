#pragma once
// PRIVATE experimental Windows-only credential access boundary.
// Never included by AXE-Voice.exe; no passwords in repository, account AOR,
// native mailbox, logs or status snapshots. No persistent secret written here.
#ifndef _WIN32
#error "Windows Credential Manager bridge is Windows-only"
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincred.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <utility>

namespace axv {

enum class SipCredentialResult {
    Consumed, InvalidTarget, WrongOwnerThread, NotFound,
    InvalidSecret, RejectedByConsumer
};

// CredentialBlob is arbitrary opaque binary data; it is NOT assumed to be
// zero-terminated or text. Callback MUST NOT retain the memory pointer.
// The native owner ID is checked on every access; UI is never passed bytes.
using SipCredentialConsumer =
    std::function<bool(const std::uint8_t*, std::size_t)>;

inline bool sip_credential_target_safe(const std::wstring& target) {
    static const std::wstring prefix = L"AXEVoice/SIP/";
    if (target.size() <= prefix.size() || target.size() > 128 ||
        target.compare(0, prefix.size(), prefix) != 0) return false;
    for (std::size_t i = prefix.size(); i < target.size(); ++i) {
        const wchar_t c = target[i];
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'-' ||
              c == L'_' || c == L'.')) return false;
    }
    return true;
}

inline SipCredentialResult consume_sip_credential_on_owner(
    const std::wstring& target, std::thread::id owner,
    const SipCredentialConsumer& consumer) {
    if (!sip_credential_target_safe(target) || !consumer)
        return SipCredentialResult::InvalidTarget;
    if (owner == std::thread::id{} || owner != std::this_thread::get_id())
        return SipCredentialResult::WrongOwnerThread;

    PCREDENTIALW handle = nullptr;
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &handle) || !handle)
        return SipCredentialResult::NotFound;

    // Never return an error string or include a credential in a diagnostic.
    // Use a narrow scope and wipe the OS-allocated blob before freeing it.
    struct Lease {
        PCREDENTIALW credential;
        ~Lease() {
            if (!credential) return;
            if (credential->CredentialBlob && credential->CredentialBlobSize)
                SecureZeroMemory(credential->CredentialBlob,
                                 credential->CredentialBlobSize);
            CredFree(credential);
        }
    } lease{handle};

    if (handle->Type != CRED_TYPE_GENERIC ||
        !handle->TargetName || target != handle->TargetName ||
        !handle->CredentialBlob || handle->CredentialBlobSize == 0 ||
        handle->CredentialBlobSize > 2048)
        return SipCredentialResult::InvalidSecret;

    bool accepted = false;
    try {
        accepted = consumer(
            reinterpret_cast<const std::uint8_t*>(handle->CredentialBlob),
            static_cast<std::size_t>(handle->CredentialBlobSize));
    } catch (...) {
        // The RAII lease still scrubs bytes even if SDK credential setup fails.
        return SipCredentialResult::RejectedByConsumer;
    }
    return accepted ? SipCredentialResult::Consumed :
                      SipCredentialResult::RejectedByConsumer;
}

} // namespace axv
