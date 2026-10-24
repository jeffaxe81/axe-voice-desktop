#pragma once
// LAB ONLY. The selected Baresip account exists only after ua_alloc(), so
// credentials must be installed AFTER UA creation and BEFORE ua_register().
// All hooks execute on the serialized libre event-loop owner thread.
#include "baresip_native_core_bootstrap.h"
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace axv {
struct NativeUaAccountCredentialHooks {
    std::function<bool(struct ua*)> install;
    // MUST clear the native SDK account's copy while UA still exists.
    // May run after a partial/failed install; MUST be idempotent and noexcept.
    std::function<void(struct ua*)> clear;
};

inline BaresipNativeRegistrationHost::NativeUaLifecycleHooks
make_account_credential_gated_ua_hooks(NativeUaIdentity identity,
                                       BaresipNativeCoreHooks core,
                                       NativeUaAccountCredentialHooks credential) {
    struct State { bool active = false; std::thread::id owner{}; };
    auto state = std::make_shared<State>();
    auto native = BaresipNativeUaFactory::make_owner_hooks(std::move(identity));
    BaresipNativeRegistrationHost::NativeUaLifecycleHooks hooks;
    hooks.create = [state, core, credential, native]() -> struct ua* {
        if (!core.prepare || !core.release || !credential.install ||
            !credential.clear || state->active) return nullptr;
        state->owner = std::this_thread::get_id();
        struct ua* selected = nullptr;
        bool attempted_install = false;
        try {
            if (core.prepare()) {
                selected = native.create();
                if (selected) {
                    attempted_install = true;
                    if (credential.install(selected)) {
                        state->active = true;
                        return selected;
                    }
                }
            }
        } catch (...) {
            // Cannot proceed after SDK failure; cleanup still mandatory.
        }
        // Failed install can still have created native SDK-owned secret bytes.
        if (selected) {
            if (attempted_install) {
                try { credential.clear(selected); } catch (...) {}
            }
            try { native.destroy(selected); } catch (...) {}
        }
        // Core release MUST also undo partial prepare() failures.
        try { core.release(); } catch (...) {}
        state->owner = {};
        return nullptr;
    };
    hooks.destroy = [state, core, credential, native](struct ua* selected) {
        if (!state->active || state->owner != std::this_thread::get_id())
            return;
        // Critical ordering: callback unsubscribe/quiescence (host) ->
        // wipe SDK's account password -> destroy UA -> release core.
        if (selected) {
            try { credential.clear(selected); } catch (...) {}
            try { native.destroy(selected); } catch (...) {}
        }
        try { core.release(); } catch (...) {}
        state->active = false;
        state->owner = {};
    };
    return hooks;
}
} // namespace axv
