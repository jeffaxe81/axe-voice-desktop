#pragma once
// LAB ONLY: private owner-thread bridge to the Baresip UA API.
// This DOES NOT initialize the Baresip core, load SIP credentials, or enable
// the AXE Voice GUI. The caller must attest and initialize the audited SDK
// BEFORE provisioning a real UA on its native event-loop thread.
#include "baresip_native_registration_host.h"
#include <baresip.h>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace axv {

struct NativeUaIdentity {
    std::string user;
    std::string domain;
    unsigned short port = 5061;
    bool signaling_tls = true;
};

// Deliberately supports only ASCII SIP identities and DNS hostnames.
// Rejects password/user-info syntax, IPv6 literal, control chars, whitespace,
// URI parameters supplied by the caller, and non-TLS signaling. There is NO
// password field here; challenge authentication needs a separate secure SDK
// account credential provider and independent release gate.
inline std::optional<std::string> native_ua_safe_aor(const NativeUaIdentity& id) {
    if (!id.signaling_tls || !id.port || id.user.empty() ||
        id.user.size() > 64 || id.domain.empty() ||
        id.domain.size() > 253) return std::nullopt;
    for (unsigned char c : id.user) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'))
            return std::nullopt;
    }
    std::size_t label = 0;
    for (std::size_t i = 0; i < id.domain.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(id.domain[i]);
        if (c == '.') {
            if (!label || label > 63 || id.domain[i - 1] == '-')
                return std::nullopt;
            label = 0;
            continue;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-')) return std::nullopt;
        if (!label && c == '-') return std::nullopt;
        if (++label > 63) return std::nullopt;
    }
    if (!label || id.domain.back() == '-') return std::nullopt;
    // No headers, auth password, remote-control directives or extra params.
    return std::string("<sip:") + id.user + "@" + id.domain + ":" +
           std::to_string(id.port) + ";transport=tls>";
}

class BaresipNativeUaFactory {
public:
    // Hooks are called by BaresipNativeRegistrationHost on its re_main owner
    // thread. This class does not expose the UA outside that thread.
    static BaresipNativeRegistrationHost::NativeUaLifecycleHooks
    make_owner_hooks(NativeUaIdentity identity) {
        BaresipNativeRegistrationHost::NativeUaLifecycleHooks hooks;
        hooks.create = [id = std::move(identity)]() -> ua* {
            const auto aor = native_ua_safe_aor(id);
            if (!aor) return nullptr; // fail-closed BEFORE invoking C SDK
            struct ua* selected = nullptr;
            const int error = ua_alloc(&selected, aor->c_str());
            if (error || !selected) {
                // A defensive rollback for SDKs that return error together
                // with a still-owned pointer. Test this behavior with fakes.
                if (selected) (void)ua_destroy(selected);
                return nullptr;
            }
            return selected;
        };
        hooks.destroy = [](ua* selected) {
            // Only called after gateway.stop(), callback quiescence and
            // no remaining SDK handler. The native SDK owns the UA bytes.
            if (selected) (void)ua_destroy(selected);
        };
        return hooks;
    }
};
} // namespace axv
