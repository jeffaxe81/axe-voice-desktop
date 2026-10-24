// BUILD-ONLY. No execution, registration or network setup.
#include "baresip_sdk_registration_runtime.h"
int main() {
    axv::ExperimentalNativeRegistrationOptions options;
    // The ordinary factory refuses by default; link its real implementation.
    auto host = axv::make_experimental_sdk_registration_host(options);
    return host ? 1 : 0;
}
