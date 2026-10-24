// BUILD-ONLY. No execution, registration or network setup.
#include "experimental_native_registration_controller.h"
#include <utility>
int main() {
    axv::ExperimentalNativeRegistrationOptions options;
    // Default opt-in is false. The factory links against the SDK-backed
    // implementation but no network or Baresip core is initialized.
    auto host = axv::make_experimental_sdk_registration_host(options);
    axv::ExperimentalNativeRegistrationController controller(std::move(host));
    const auto state = controller.snapshot();
    return (state.sip_authenticated || state.media_verified) ? 1 : 0;
}
