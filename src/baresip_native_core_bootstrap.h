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

// This composes with BaresipNativeRegistrationHost's private owner hooks.
// No sensitive material or UA pointer crosses the UI command mailbox.
// All callbacks run from the existing libre owner thread.
inline BaresipNativeRegistrationHost::NativeUaLifecycleHooks
make_core_gated_native_ua_hooks(NativeUaIdentity identity,
                                BaresipNativeCoreHooks core) {
    struct State {
        bool core_live = false;
        std::thread::id owner{};
    };
    auto state = std::make_shared<State>();
    auto native = BaresipNativeUaFactory::make_owner_hooks(std::move(identity));
    BaresipNativeRegistrationHost::NativeUaLifecycleHooks result;

    result.create = [state, core, native]() -> ua* {
        // Missing or mismatched core lifecycle is a hard refusal, not a
        // fallback to direct ua_alloc. Never provision a second live UA.
        if (!core.prepare || !core.release || state->core_live)
            return nullptr;
        state->owner = std::this_thread::get_id();
        bool prepared = false;
        try {
            prepared = core.prepare();
            if (prepared) {
                auto* selected = native.create();
                if (selected) {
                    state->core_live = true;
                    return selected;
                }
            }
        } catch (...) {
            // The host's start() must fail closed even if setup throws.
        }
        // release() is REQUIRED to handle partial preparation, including
        // prepare() returning false or throwing mid-initialization.
        try { core.release(); } catch (...) { /* no SDK reuse on this run */ }
        state->owner = {};
        return nullptr;
    };

    result.destroy = [state, core, native](ua* selected) {
        // The host always executes this after unregister and callback
        // quiescence, on the same thread as create(). No cross-thread
        // SDK teardown is permitted.
        if (!state->core_live || state->owner != std::this_thread::get_id())
            return;
        try { native.destroy(selected); }
        catch (...) { /* Still release the core, do not propagate. */ }
        try { core.release(); }
        catch (...) { /* release() is contractually noexcept. */ }
        state->core_live = false;
        state->owner = {};
    };
    return result;
}

} // namespace axv
