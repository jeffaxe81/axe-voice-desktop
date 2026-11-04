#include "../src/sip_runtime_observation.h"
#include "../src/sip_indicator_tone.h"
#include <string_view>
int main() {
    using namespace axv;
    SipRuntimeObservation o;
    if (effective_sip_status(o, 1) != SipStatus::Unavailable) return 1;
    if (may_use_call_control(o, 1, CallAction::Dial)) return 2;
    if (std::wstring_view(runtime_status_banner(o, 1)).find(L"PARADO") == std::wstring_view::npos) return 3;
    o.process = RuntimeProcess::Running;
    o.sip = SipStatus::Registered; o.session = 21;
    if (effective_sip_status(o, 21) != SipStatus::Unavailable) return 4;
    if (may_use_call_control(o, 21, CallAction::Dial)) return 5;
    if (std::wstring_view(runtime_status_banner(o, 21)).find(L"NAO CONFIRMADO") == std::wstring_view::npos) return 6;
    // Policy-only synthetic native evidence; no production adapter exists yet.
    o.evidence = SipEvidence::VerifiedNativeApi;
    if (effective_sip_status(o, 22) != SipStatus::Unavailable) return 7;
    if (may_use_call_control(o, 22, CallAction::Dial)) return 8;
    if (!may_use_call_control(o, 21, CallAction::Dial)) return 9;
    if (may_use_call_control(o, 21, CallAction::Hangup)) return 10;
    o.sip = SipStatus::InCall;
    if (!may_use_call_control(o, 21, CallAction::Hangup)) return 11;
    if (may_use_call_control(o, 21, CallAction::Dial)) return 12;
    o.process = RuntimeProcess::Stopping;
    if (may_use_call_control(o, 21, CallAction::Hangup)) return 13;
    if (std::wstring_view(runtime_status_banner(o, 21)).find(L"ENCERRANDO") == std::wstring_view::npos) return 14;
    o.process = RuntimeProcess::Failed;
    if (may_use_call_control(o, 21, CallAction::Hangup)) return 15;
    o.process = RuntimeProcess::Stopped;
    if (effective_sip_status(o, 21) != SipStatus::Unavailable) return 16;
    if (!is_stale_session(0, 21) || !is_stale_session(20, 21) || is_stale_session(21, 21)) return 17;
    // Traffic-light policy must not claim a connection based on process state.
    if (sip_indicator_tone({}, 1) != SipIndicatorTone::Red) return 18;
    SipRuntimeObservation light{RuntimeProcess::Running, SipStatus::Registered,
                                SipEvidence::None, 9, 0, 0};
    if (sip_indicator_tone(light, 9) != SipIndicatorTone::Amber) return 19;
    light.evidence = SipEvidence::VerifiedNativeApi;
    if (sip_indicator_tone(light, 10) != SipIndicatorTone::Amber) return 20;
    if (sip_indicator_tone(light, 9) != SipIndicatorTone::Green) return 21;
    for (auto status : {SipStatus::Calling, SipStatus::Ringing, SipStatus::InCall}) {
        light.sip = status;
        if (sip_indicator_tone(light, 9) != SipIndicatorTone::Green) return 22;
    }
    light.sip = SipStatus::Registering;
    if (sip_indicator_tone(light, 9) != SipIndicatorTone::Amber) return 23;
    for (auto status : {SipStatus::Disconnected, SipStatus::Failed}) {
        light.sip = status;
        if (sip_indicator_tone(light, 9) != SipIndicatorTone::Red) return 24;
    }
    light.process = RuntimeProcess::Stopped;
    light.sip = SipStatus::Registered;
    if (sip_indicator_tone(light, 9) != SipIndicatorTone::Red) return 25;
    return 0;
}
