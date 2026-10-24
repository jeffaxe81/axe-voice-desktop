// BUILD-ONLY. No execution, registration or network setup.
#include "experimental_native_registration_controller.h"
#include <baresip.h>
#include <type_traits>
#include <utility>

// Compile-only ABI drift guard for the exact public SDK functions used when
// the native UA's account receives and then scrubs the WinCred secret.
static_assert(std::is_same_v<decltype(&ua_account),
                             struct account* (*)(const struct ua*)>,
              "Audited Baresip SDK changed ua_account ABI");
static_assert(std::is_same_v<decltype(&account_set_auth_pass),
                             int (*)(struct account*, const char*)>,
              "Audited Baresip SDK changed account_set_auth_pass ABI");
static_assert(std::is_same_v<decltype(&account_auth_pass),
                             const char* (*)(const struct account*)>,
              "Audited Baresip SDK changed account_auth_pass ABI");
int main() {
    axv::ExperimentalNativeRegistrationOptions options;
    // Default opt-in is false. The factory links against the SDK-backed
    // implementation but no network or Baresip core is initialized.
    auto host = axv::make_experimental_sdk_registration_host(options);
    axv::ExperimentalNativeRegistrationController controller(std::move(host));
    const auto state = controller.snapshot();
    return (state.sip_authenticated || state.media_verified) ? 1 : 0;
}
