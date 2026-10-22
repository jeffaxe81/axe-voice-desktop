#include "../src/baresip_event_reducer.h"
#include <string_view>

int main() {
    using namespace axv;
    SipRuntimeObservation o{RuntimeProcess::Running, SipStatus::Unavailable,
                            SipEvidence::None, 31};
    auto send = [&](BaresipEventKind kind, std::uint64_t session = 31,
                    std::uint64_t call_id = 0) {
        return apply_baresip_event(o, 31, {kind, session, call_id});
    };
    if (send(BaresipEventKind::Registering) != EventTransition::UnverifiedSource) return 1;
    if (o.sip != SipStatus::Unavailable) return 2;
    // Synthetic verified events exercise policy only; no real backend uses this today.
    o.evidence = SipEvidence::VerifiedNativeApi;
    if (send(BaresipEventKind::RegisterOk) != EventTransition::InvalidOrder) return 3;
    if (send(BaresipEventKind::Registering) != EventTransition::Applied) return 4;
    if (send(BaresipEventKind::RegisterOk, 30) != EventTransition::StaleSession) return 5;
    if (o.sip != SipStatus::Registering) return 6;
    if (send(BaresipEventKind::RegisterOk) != EventTransition::Applied) return 7;
    if (!may_use_call_control(o, 31, CallAction::Dial)) return 8;
    if (may_use_call_control(o, 31, CallAction::Answer)) return 9;

    if (send(BaresipEventKind::CallEstablished) != EventTransition::InvalidOrder) return 10;
    if (send(BaresipEventKind::CallIncoming, 31, 100) != EventTransition::Applied) return 11;
    if (o.sip != SipStatus::Ringing || !may_use_call_control(o, 31, CallAction::Answer)) return 12;
    if (may_use_call_control(o, 31, CallAction::Dial)) return 13;
    if (send(BaresipEventKind::CallEstablished, 31, 100) != EventTransition::Applied) return 14;
    if (o.sip != SipStatus::InCall || !may_use_call_control(o, 31, CallAction::Hangup)) return 15;
    if (send(BaresipEventKind::CallClosed, 31, 100) != EventTransition::Applied) return 16;
    if (o.sip != SipStatus::Registered || o.call_id != 0 || o.last_call_id != 100) return 17;
    if (send(BaresipEventKind::CallOutgoing, 31, 101) != EventTransition::Applied) return 18;
    if (o.sip != SipStatus::Calling || !may_use_call_control(o, 31, CallAction::Hangup)) return 19;
    if (send(BaresipEventKind::CallClosed, 31, 101) != EventTransition::Applied) return 20;
    if (o.call_id != 0 || o.last_call_id != 101) return 201;
    if (send(BaresipEventKind::Registering) != EventTransition::Applied) return 21;
    if (send(BaresipEventKind::RegisterFail) != EventTransition::Applied) return 22;
    if (o.sip != SipStatus::Failed) return 23;
    if (send(BaresipEventKind::Registering) != EventTransition::Applied) return 24;
    if (send(BaresipEventKind::RegisterOk) != EventTransition::Applied) return 25;
    if (send(BaresipEventKind::Unregistering) != EventTransition::Applied) return 26;
    if (o.sip != SipStatus::Disconnected) return 27;
    o.process = RuntimeProcess::Stopping;
    if (send(BaresipEventKind::Registering) != EventTransition::UnverifiedSource) return 28;
    if (may_use_call_control(o, 31, CallAction::Dial)) return 29;
    // Verify that restart cannot inherit old call state despite old event payloads.
    o.process = RuntimeProcess::Running;
    o.session = 32;
    if (send(BaresipEventKind::Registering) != EventTransition::StaleSession) return 30;
    return 0;
}
