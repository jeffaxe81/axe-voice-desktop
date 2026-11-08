#include "../src/release_scope_policy.h"
#include <cstdint>
#include <cwchar>

int main() {
    using namespace axv;
    if (kRequireRealTelephonyForEngineeringDelivery ||
        !kRequireRealTelephonyForProduction) return 1;
    if (!std::wcsstr(kReleaseScopeDiagnostic, L"NAO APROVADOS") ||
        !std::wcsstr(kReleaseScopeDiagnostic, L"PRODUCAO: BLOQUEADA")) return 2;
    // Exhaustively prove that none of the 8 software gates may be omitted.
    for (std::uint32_t mask = 0; mask < 256; ++mask) {
        const EngineeringReleaseEvidence engineering{
            bool(mask&1), bool(mask&2), bool(mask&4), bool(mask&8),
            bool(mask&16), bool(mask&32), bool(mask&64), bool(mask&128)
        };
        const auto no_external = evaluate_release_scope(engineering);
        if (no_external.engineering_delivery_ready != (mask == 255) ||
            no_external.production_approved ||
            !no_external.live_telephony_deferred_from_engineering) return 3;
        // Exhaustively prove that production additionally requires all
        // 6 external checks, including PBX, media, OS, crypto and legal.
        for (std::uint32_t external_mask = 0; external_mask < 64; ++external_mask) {
            const ExternalReleaseEvidence external{
                bool(external_mask&1), bool(external_mask&2),
                bool(external_mask&4), bool(external_mask&8),
                bool(external_mask&16), bool(external_mask&32)
            };
            const auto decision = evaluate_release_scope(engineering, external);
            if (decision.engineering_delivery_ready != (mask == 255) ||
                decision.production_approved != (mask == 255 && external_mask == 63))
                return 4;
        }
    }
    // The 100% software scenario is valid with no external PBX evidence.
    const EngineeringReleaseEvidence complete{true,true,true,true,true,true,true,true};
    const auto candidate = evaluate_release_scope(complete, {});
    if (!candidate.engineering_delivery_ready || candidate.production_approved) return 5;
    return 0;
}
