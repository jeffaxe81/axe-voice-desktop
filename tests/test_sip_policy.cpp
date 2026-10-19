#include "../src/sip_status.h"
#include "../src/codec_policy.h"
#include "../src/sip_diagnostics.h"
#include "../src/call_policy.h"
#include <string_view>
int main() {
    using namespace axv;
    if (std::wstring_view(sip_status_label(SipStatus::Registered)) != L"Registrado") return 1;
    if (std::wstring_view(sip_status_label(SipStatus::Unavailable)).empty()) return 2;
    if (default_codec_capabilities.size() != 5) return 3;
    if (can_enable_codec(Codec::G729, false)) return 4;
    if (!can_enable_codec(Codec::Opus, false)) return 5;
    if (!can_enable_codec(Codec::G729, true)) return 6;
    if (std::wstring_view(diagnostic_text(DiagnosticCode::EngineMissing)).find(L"AXV-SIP-001") == std::wstring_view::npos) return 7;
    if (can_apply_call_action(SipStatus::Unavailable, CallAction::Dial)) return 8;
    if (!can_apply_call_action(SipStatus::Registered, CallAction::Dial)) return 9;
    if (can_apply_call_action(SipStatus::Disconnected, CallAction::Answer)) return 10;
    if (!can_apply_call_action(SipStatus::InCall, CallAction::Hangup)) return 11;
    return 0;
}
