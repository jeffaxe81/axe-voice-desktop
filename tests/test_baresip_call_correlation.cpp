#include "../src/sip_event_projection.h"

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    const auto a = mailbox.begin_session();
    // Synthetic proof is for policy tests only; no real Baresip adapter exists.
    if (!mailbox.enqueue({BaresipEventKind::Registering, a}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, a})) return 1;
    auto state = projection.observe(RuntimeProcess::Running,
                                    SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered || state.call_id) return 2;

    // A call cannot start without an opaque call token.
    if (!mailbox.enqueue({BaresipEventKind::CallIncoming, a, 0})) return 3;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Unavailable || state.evidence != SipEvidence::None) return 4;

    // Start a fresh native generation after fail-closed rejection.
    const auto b = mailbox.begin_session();
    if (!mailbox.enqueue({BaresipEventKind::Registering, b}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, b}) ||
        !mailbox.enqueue({BaresipEventKind::CallIncoming, b, 40}) ||
        !mailbox.enqueue({BaresipEventKind::CallEstablished, b, 40}) ||
        !mailbox.enqueue({BaresipEventKind::CallClosed, b, 40}) ||
        !mailbox.enqueue({BaresipEventKind::CallOutgoing, b, 41})) return 5;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Calling || state.call_id != 41 ||
        state.last_call_id != 41) return 6;

    // Late CallClosed from call 40 MUST NOT terminate the new call 41.
    if (!mailbox.enqueue({BaresipEventKind::CallClosed, b, 40})) return 7;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Unavailable || state.session != 0 ||
        may_use_call_control(state, b, CallAction::Hangup)) return 8;
    if (!mailbox.enqueue({BaresipEventKind::CallEstablished, b, 41})) return 9;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.evidence != SipEvidence::None) return 10;

    const auto c = mailbox.begin_session();
    if (!mailbox.enqueue({BaresipEventKind::Registering, c}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, c}) ||
        !mailbox.enqueue({BaresipEventKind::CallOutgoing, c, 9}) ||
        !mailbox.enqueue({BaresipEventKind::CallClosed, c, 9}) ||
        !mailbox.enqueue({BaresipEventKind::CallOutgoing, c, 9})) return 11;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Unavailable || state.evidence != SipEvidence::None) return 12;

    // A transport loss revokes the active call without wiping the high-water ID.
    SipRuntimeObservation direct{RuntimeProcess::Running, SipStatus::Registered,
                                 SipEvidence::VerifiedNativeApi, 999};
    if (apply_baresip_event(direct, 999, {BaresipEventKind::CallOutgoing, 999, 7}) !=
        EventTransition::Applied) return 13;
    if (apply_baresip_event(direct, 999, {BaresipEventKind::TransportLost, 999}) !=
        EventTransition::Applied || direct.call_id || direct.last_call_id != 7) return 14;
    if (apply_baresip_event(direct, 999, {BaresipEventKind::Registering, 999}) !=
        EventTransition::Applied) return 15;
    if (apply_baresip_event(direct, 999, {BaresipEventKind::RegisterOk, 999}) !=
        EventTransition::Applied) return 16;
    if (apply_baresip_event(direct, 999, {BaresipEventKind::CallOutgoing, 999, 7}) !=
        EventTransition::StaleCall) return 17;
    return 0;
}
