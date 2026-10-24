#pragma once
// Isolated SDK-LAB Credential Manager ingress. Never included in AXEVoice.exe.
// A credential byte lease is available ONLY during the native re_main owner
// callback. The real SDK adapter must copy/install securely and explicitly
// scrub its own memory in revoke(). No SIP success claim is made here.
#include "baresip_native_core_bootstrap.h"
#include "sip_vault_status_win32.h"
#include "sip_credential_lease_win32.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace axv {
struct NativeSdkCredentialPort {
    // These two hooks MUST be implemented and audited against the actual
    // Baresip SDK ABI; neither a bool stub nor secret retention is sufficient.
    std::function<bool(const std::uint8_t*, std::size_t)> install;
    std::function<void()> revoke;
};

inline std::optional<NativeCredentialLifecycleHooks>
make_native_owner_vault_hooks(const NativeUaIdentity& identity,
                             NativeSdkCredentialPort port) {
    // Refuse missing callback or invalid identity before touching the vault.
    if (!port.install || !port.revoke || !native_ua_safe_aor(identity))
        return std::nullopt;
    const std::wstring user(identity.user.begin(), identity.user.end());
    const std::wstring host(identity.domain.begin(), identity.domain.end());
    const auto target = sip_vault_alias(user, host);
    if (target.empty()) return std::nullopt;

    NativeCredentialLifecycleHooks hooks;
    hooks.install = [target, install = std::move(port.install)]() {
        return consume_sip_credential_on_owner(
            target, std::this_thread::get_id(), install) ==
            SipCredentialResult::Consumed;
    };
    hooks.revoke = [revoke = std::move(port.revoke)]() {
        // Called even after partial failure of installation.
        revoke();
    };
    return hooks;
}
} // namespace axv
