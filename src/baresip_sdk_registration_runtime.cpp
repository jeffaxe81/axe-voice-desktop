#include "baresip_sdk_registration_runtime.h"
#include "baresip_native_core_bootstrap.h"
#include "baresip_sdk_account_credentials_win32.h"
#include <utility>

namespace axv {
std::unique_ptr<BaresipNativeRegistrationHost>
make_experimental_sdk_registration_host(ExperimentalNativeRegistrationOptions options) {
#if !defined(AXV_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME) || \
    AXV_EXPERIMENTAL_NATIVE_REGISTRATION_RUNTIME != 1 || \
    !defined(AXV_EXPERIMENTAL_NATIVE_WINCREDS_LAB) || \
    AXV_EXPERIMENTAL_NATIVE_WINCREDS_LAB != 1
    (void)options;
    return {};
#else
    // This opt-in is NOT the commercial release gate and can never be used as
    // VerifiedNativeApi or permission to originate calls in the GUI.
    if (!options.explicitly_enabled_for_isolated_lab ||
        !options.tls_and_sdk_reviewed_externally ||
        !options.core_policy.enable_experimental_native_core ||
        !options.core_policy.sdk_integrity_reviewed ||
        !options.sdk_account_copy_and_scrub_reviewed ||
        !native_ua_safe_aor(options.identity))
        return {};

    // Real account_set_auth_pass must execute AFTER ua_alloc returns a UA and
    // BEFORE ua_register. No secret goes into AOR, registry config or UI.
    // Source-level review of the audited Baresip account copy/free ABI is
    // required because we explicitly scrub the account-owned heap copy.
    auto scoped = make_baresip_account_wincred_hooks(options.identity);
    if (!scoped) return {};

    auto core = make_experimental_sdk_core_hooks(std::move(options.core_policy));
    auto hooks = make_account_credential_gated_ua_hooks(
        std::move(options.identity), std::move(core), std::move(*scoped));
    return std::make_unique<BaresipNativeRegistrationHost>(std::move(hooks));
#endif
}
} // namespace axv
