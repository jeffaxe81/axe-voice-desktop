#pragma once
// EXPERIMENTAL / SDK-AUDIT-ONLY. This API is NOT referenced by AXEVoice.exe.
// This is a composition of the existing libre owner, Baresip SDK core hooks
// and UA lifecycle: it NEVER confers authenticated SIP registration or media.
#include "baresip_native_registration_host.h"
#include "baresip_native_ua_factory.h"
#include "baresip_sdk_core_bootstrap.h"
#include "sip_native_owner_credential_bridge.h"
#include <memory>

namespace axv {
struct ExperimentalNativeRegistrationOptions {
    NativeUaIdentity identity{};
    BaresipSdkCorePolicy core_policy{};
    bool explicitly_enabled_for_isolated_lab = false;
    bool tls_and_sdk_reviewed_externally = false;
    // MUST be backed by audited SDK functions: no default dummy secret
    // callback is allowed to start a registration host in this build.
    NativeSdkCredentialPort credential_port{};
};

// Returns null unless explicitly opted into *isolated* SDK development.
// The caller must serialize start/submit/cancel/stop on a non-owner thread.
// The owner thread alone creates, registers and destroys the UA.
std::unique_ptr<BaresipNativeRegistrationHost>
make_experimental_sdk_registration_host(ExperimentalNativeRegistrationOptions options);
} // namespace axv
