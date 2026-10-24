#pragma once
// EXPERIMENTAL / NOT SHIPPED / NOT LINKED INTO AXE-Voice.exe.
// Real Baresip SDK bootstrap for an already audited SDK build. Requires
// explicit opt-in AND an isolated pre-inspected LAB profile. No credentials.
// No call here proves TLS, authentication, SRTP, or operational SIP readiness.
#include "baresip_native_core_bootstrap.h"
#include "baresip_profile.h"
#include <baresip.h>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

namespace axv {

struct BaresipSdkCorePolicy {
    bool enable_experimental_native_core = false; // deny by default
    bool sdk_integrity_reviewed = false;            // external attestations
    std::filesystem::path isolated_profile;
};

// These hooks run on BaresipNativeRegistrationHost's own libre re_main thread.
// The containing reactor has already called libre_init() successfully.
// Caller must call release() on the same thread if prepare() fails or succeeds.
inline BaresipNativeCoreHooks
make_experimental_sdk_core_hooks(BaresipSdkCorePolicy policy) {
    struct State {
        bool configured = false;
        bool baresip_started = false;
        bool ua_started = false;
        bool modules_started = false;
        bool ready = false;
    };
    auto state = std::make_shared<State>();
    BaresipNativeCoreHooks hooks;

    hooks.prepare = [state, policy]() -> bool {
        // No implicit fallback to the system-wide ~/.baresip config.
        if (!policy.enable_experimental_native_core ||
            !policy.sdk_integrity_reviewed ||
            state->configured || state->baresip_started ||
            state->ua_started || state->ready ||
            policy.isolated_profile.empty())
            return false;

        if (prepare_baresip_lab_profile(policy.isolated_profile)
                != BaresipProfileStatus::Ready)
            return false;

        // Current Baresip C API accepts a narrow path. Refuse non-ASCII
        // rather than silently changing a Windows Unicode profile path.
        const auto native_path = policy.isolated_profile.generic_string();
        if (native_path.empty() || native_path.size() > 240)
            return false;
        for (unsigned char c : native_path)
            if (c < 0x20 || c > 0x7e || c == '"' || c == '\'')
                return false;

        if (conf_path_set(native_path.c_str()) != 0)
            return false;
        // Every successful stage is recorded for reverse-order rollback.
        state->configured = true;
        if (conf_configure() != 0)
            return false;
        if (!conf_config())
            return false;
        if (baresip_init(conf_config()) != 0)
            return false;
        state->baresip_started = true;
        if (ua_init("AXE Voice LAB", true, true, true) != 0)
            return false;
        state->ua_started = true;
        // Modules are loaded ONLY from the validated isolated LAB profile;
        // a separate allowlist/review is still mandatory before production.
        if (conf_modules() != 0)
            return false;
        state->modules_started = true;
        state->ready = true;
        return true;
    };

    hooks.release = [state]() {
        // This hook runs after UA destroy and native callback quiescence.
        // Failed partial startup also reaches here through the v0.69 gate.
        if (state->ua_started) {
            ua_close();
            state->ua_started = false;
        }
        if (state->configured) {
            module_app_unload();
            conf_close();
        }
        if (state->baresip_started) {
            baresip_close();
            state->baresip_started = false;
        }
        if (state->configured) {
            mod_close();
            state->configured = false;
        }
        state->modules_started = false;
        state->ready = false;
    };
    return hooks;
}
} // namespace axv
