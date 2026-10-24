#include "baresip_sdk_registration_runtime.h"
#include "baresip_native_core_bootstrap.h"
#include <utility>

namespace axv {
std::unique_ptr<BaresipNativeRegistrationHost>
make_experimental_sdk_registration_host(ExperimentalNativeRegistrationOptions options) {
#if !defined(AXV_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME) || \
    AXV_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME != 1
    (void)options;
    return {};
#else
    // This opt-in is NOT the commercial release gate and can never be used as
    // VerifiedNativeApi or permission to originate calls in the GUI.
    if (!options.explicitly_enabled_for_isolated_lab ||
        !options.tls_and_sdk_reviewed_externally ||
        !options.core_policy.enable_experimental_native_core ||
        !options.core_policy.sdk_integrity_reviewed ||
        !native_ua_safe_aor(options.identity))
        return {};

    // Core prepare/release and UA create/destroy execute only on the native
    // libre re_main owner thread. The public handle/queue holds no secrets.
    auto core = make_experimental_sdk_core_hooks(std::move(options.core_policy));
    auto hooks = make_core_gated_native_ua_hooks(std::move(options.identity),
                                                 std::move(core));
    return std::make_unique<BaresipNativeRegistrationHost>(std::move(hooks));
#endif
}
} // namespace axv
