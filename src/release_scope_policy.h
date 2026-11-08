#pragma once
// AXE Voice release acceptance contract (2026-11-08).
// The owner explicitly DEFERRED live-PBX and bidirectional-RTP testing from
// the *software engineering delivery* scope. This is NOT an interoperability
// approval and must NEVER be interpreted as permission for production.
// Keep this guard independent of SIP credentials, accounts and UI state.
#include <cstdint>

namespace axv {

inline constexpr bool kRequireRealTelephonyForEngineeringDelivery = false;
inline constexpr bool kRequireRealTelephonyForProduction = true;

struct EngineeringReleaseEvidence {
    bool windows_build_and_ctest = false;
    bool upstream_integrity = false;
    bool native_sdk_abi_and_reactor = false;
    bool sip_loopback_1000 = false;
    bool native_installer_acceptance = false;
    bool win32_compact_ui = false;
    bool winui3_lab_candidate = false;
    bool credentials_and_security_checks = false;
};

struct ExternalReleaseEvidence {
    bool live_pbx_register = false;
    bool live_incoming_outgoing_calls = false;
    bool live_bidirectional_audio = false;
    bool headset_and_windows_physical = false;
    bool deployment_crypto_and_codecs = false;
    bool legal_licensing_and_signing = false;
};

struct ReleaseDecision {
    bool engineering_delivery_ready = false;
    bool production_approved = false;
    bool live_telephony_deferred_from_engineering = true;
};

constexpr bool all_engineering_checks_pass(const EngineeringReleaseEvidence& e) noexcept {
    return e.windows_build_and_ctest &&
           e.upstream_integrity &&
           e.native_sdk_abi_and_reactor &&
           e.sip_loopback_1000 &&
           e.native_installer_acceptance &&
           e.win32_compact_ui &&
           e.winui3_lab_candidate &&
           e.credentials_and_security_checks;
}

constexpr bool all_external_checks_pass(const ExternalReleaseEvidence& e) noexcept {
    return e.live_pbx_register &&
           e.live_incoming_outgoing_calls &&
           e.live_bidirectional_audio &&
           e.headset_and_windows_physical &&
           e.deployment_crypto_and_codecs &&
           e.legal_licensing_and_signing;
}

constexpr ReleaseDecision evaluate_release_scope(
    const EngineeringReleaseEvidence& engineering,
    const ExternalReleaseEvidence& external = {}) noexcept {
    const bool ready = all_engineering_checks_pass(engineering);
    // Production always requires physical and network evidence, regardless
    // of the owner's decision to defer it from software engineering.
    return {ready, ready && all_external_checks_pass(external), true};
}

// A privacy-safe fixed statement for F2/COPIAR STATUS. No account information.
inline constexpr wchar_t kReleaseScopeDiagnostic[] =
    L"ACEITE DE SOFTWARE: testes de engenharia e pacotes independem da central real.\n"
    L"HOMOLOGACAO EXTERNA: registro/chamadas/RTP reais adiados, NAO APROVADOS.\n"
    L"PRODUCAO: BLOQUEADA ate PBX, headset, seguranca, licencas e assinatura.";

static_assert(!kRequireRealTelephonyForEngineeringDelivery &&
              kRequireRealTelephonyForProduction,
              "Live PBX is deferred only for engineering, never for production");
static_assert(!evaluate_release_scope({}, {}).engineering_delivery_ready &&
              !evaluate_release_scope({}, {}).production_approved,
              "Unknown or missing evidence always defaults to not approved");
} // namespace axv
