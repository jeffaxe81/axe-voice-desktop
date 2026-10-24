#pragma once
// LAB-only native Baresip core readiness barrier. No real core initialization
// is assumed or performed by this header. The audited SDK bootstrap must
// supply both callbacks; returning true is a trusted, testable assertion.
#include "baresip_native_ua_factory.h"
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace axv {

// The embedder MUST perform conf configuration, baresip core initialization,
// UA subsystem initialization and audited TLS policy setup in prepare().
// release() must undo ANY partial preparation and be noexcept.
struct BaresipNativeCoreHooks {
    std::function<bool()> prepare;
    std::function<void()> release;
};

// The audited SDK integrator installs a Credential Manager secret ONLY on
// the native re_main thread. revoke() MUST purge all native SDK secret copies.
// These callbacks are NOT a real Baresip authentication implementation.
struct NativeCredentialLifecycleHooks {
    std::function<bool()> install;
    std::function<void()> revoke;
};

// This composes with BaresipNativeRegistrationHost's private owner hooks.
// No sensitive material or UA pointer crosses the UI command mailbox.
// All callbacks run from the existing libre owner thread.
inline BaresipNativeRegistrationHost::NativeUaLifecycleHooks
make_core_gated_native_ua_hooks(NativeUaIdentity identity,
                                BaresipNativeCoreHooks core,
                                NativeCredentialLifecycleHooks credentials = {}) {
    struct State {
        bool core_live = false;
        bool credentials_live = false;
        std::thread::id owner{};
    };
    auto state = std::make_shared<State>();
    auto native = BaresipNativeUaFactory::make_owner_hooks(std::move(identity));
    BaresipNativeRegistrationHost::NativeUaLifecycleHooks result;

    result.create = [state, core, native, credentials]() -> ua* {
        // Refuse incomplete credential lifecycle BEFORE starting the SDK.
        if (!core.prepare || !core.release || state->core_live ||
            static_cast<bool>(credentials.install) !=
            static_cast<bool>(credentials.revoke))
            return nullptr;
        state->owner = std::this_thread::get_id();
        bool install_attempted = false;
        try {
            if (core.prepare()) {
                bool allowed = true;
                if (credentials.install) {
                    install_attempted = true;
                    allowed = credentials.install();
                }
                if (allowed) {
                    auto* selected = native.create();
                    if (selected) {
                        state->core_live = true;
                        state->credentials_live = install_attempted;
                        return selected;
                    }
                }
            }
        } catch (...) {
            // The host's start() must fail closed and release partial setup.
        }
        // SDK credentials may have been partially installed despite a false
        // return/exception. Revoke BEFORE releasing the core even on failure.
        if (install_attempted) {
            try { credentials.revoke(); } catch (...) {}
        }
        try { core.release(); } catch (...) {}
        state->credentials_live = false;
        state->owner = {};
        return nullptr;
    };

    result.destroy = [state, core, native, credentials](ua* selected) {
        // UA teardown > credential purge > core teardown. The same owner
        // thread performs the three operations after callback quiescence.
        if (!state->core_live || state->owner != std::this_thread::get_id())
            return;
        try { native.destroy(selected); } catch (...) {}
        if (state->credentials_live) {
            try { credentials.revoke(); } catch (...) {}
        }
        try { core.release(); } catch (...) {}
        state->credentials_live = false;
        state->core_live = false;
        state->owner = {};
    };
    return result;
}

} // namespace axv
