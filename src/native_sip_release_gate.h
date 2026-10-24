#pragma once
// Defense-in-depth release gate for future native SIP integration.
// Intentionally *not wired to AXEVoice.exe*: the app continues using
// UnavailableSipEngine. This header alone does not prove any test or approval.
//
// A developer cannot enable commercial SIP merely by passing true booleans:
// an explicit audited-build compile flag is also required, and is OFF in all
// current AXE Voice build configurations.
#include <cstdint>

namespace axv {

struct NativeSipReleaseEvidence {
    bool sdk_source_hash_approved = false;
    bool native_abi_linked = false;
    bool core_bootstrap_reviewed = false;
    bool secure_credentials_reviewed = false;
    bool registrar_auth_success_and_failure_tested = false;
    bool tls_peer_certificate_verified = false;
    bool srtp_media_negotiation_verified = false;
    bool two_way_calls_pabx_verified = false;
    bool headset_routes_and_failover_verified = false;
    bool deployment_security_reviewed = false;
    bool explicit_release_approval = false;
    // Deliberately no passwords, telephone identifiers or log messages.
};

enum class NativeSipActivationDecision {
    DisabledInLabBuild,
    SdkReviewMissing,
    CoreReviewMissing,
    CredentialReviewMissing,
    RegistrationEvidenceMissing,
    TlsPeerValidationMissing,
    SrtpMediaEvidenceMissing,
    EndToEndCallEvidenceMissing,
    AudioHardwareEvidenceMissing,
    SecurityReviewMissing,
    ReleaseApprovalMissing,
    EligibleForFurtherIntegration
};

inline NativeSipActivationDecision evaluate_native_sip_activation(
    const NativeSipReleaseEvidence& evidence) noexcept {
#if !defined(AXV_ENABLE_AUDITED_NATIVE_SIP) || AXV_ENABLE_AUDITED_NATIVE_SIP != 1
    (void)evidence;
    return NativeSipActivationDecision::DisabledInLabBuild;
#else
    if (!evidence.sdk_source_hash_approved || !evidence.native_abi_linked)
        return NativeSipActivationDecision::SdkReviewMissing;
    if (!evidence.core_bootstrap_reviewed)
        return NativeSipActivationDecision::CoreReviewMissing;
    if (!evidence.secure_credentials_reviewed)
        return NativeSipActivationDecision::CredentialReviewMissing;
    if (!evidence.registrar_auth_success_and_failure_tested)
        return NativeSipActivationDecision::RegistrationEvidenceMissing;
    if (!evidence.tls_peer_certificate_verified)
        return NativeSipActivationDecision::TlsPeerValidationMissing;
    if (!evidence.srtp_media_negotiation_verified)
        return NativeSipActivationDecision::SrtpMediaEvidenceMissing;
    if (!evidence.two_way_calls_pabx_verified)
        return NativeSipActivationDecision::EndToEndCallEvidenceMissing;
    if (!evidence.headset_routes_and_failover_verified)
        return NativeSipActivationDecision::AudioHardwareEvidenceMissing;
    if (!evidence.deployment_security_reviewed)
        return NativeSipActivationDecision::SecurityReviewMissing;
    if (!evidence.explicit_release_approval)
        return NativeSipActivationDecision::ReleaseApprovalMissing;
    // Eligibility is NOT a native registration-proof token; a separately
    // audited engine still needs authenticated callback and media evidence.
    return NativeSipActivationDecision::EligibleForFurtherIntegration;
#endif
}

inline bool native_sip_is_lab_blocked() noexcept {
#if !defined(AXV_ENABLE_AUDITED_NATIVE_SIP) || AXV_ENABLE_AUDITED_NATIVE_SIP != 1
    return true;
#else
    return false;
#endif
}

} // namespace axv
