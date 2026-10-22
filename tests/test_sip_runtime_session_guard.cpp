#include "../src/sip_runtime_session_guard.h"

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    SipRuntimeSessionGuard guard;
    auto state = guard.observe(RuntimeProcess::Running, SipEvidence::None, mailbox, projection);
    if (state.session || state.sip != SipStatus::Unavailable) return 1;
    // Synthetic VerifiedNativeApi is TEST ONLY. There is no production adapter.
    const auto a = mailbox.begin_session();
    if (!mailbox.enqueue({BaresipEventKind::Registering, a}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, a})) return 2;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.sip != SipStatus::Registered || !may_use_call_control(state, a, CallAction::Dial)) return 3;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.sip != SipStatus::Registered) return 4; // persists between ticks
    if (!mailbox.enqueue({BaresipEventKind::CallOutgoing, a})) return 5;
    state = guard.observe(RuntimeProcess::Failed, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.session || state.evidence != SipEvidence::None ||
        may_use_call_control(state, a, CallAction::Dial)) return 6;
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, a})) return 7;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.session || state.sip != SipStatus::Unavailable) return 8;
    const auto b = mailbox.begin_session();
    if (b <= a || mailbox.enqueue({BaresipEventKind::RegisterOk, a})) return 9;
    if (!mailbox.enqueue({BaresipEventKind::Registering, b}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, b})) return 10;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.sip != SipStatus::Registered) return 11;
    guard.revoke(RuntimeProcess::Stopping, mailbox, projection);
    if (mailbox.enqueue({BaresipEventKind::CallIncoming, b})) return 12;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.session || may_use_call_control(state, b, CallAction::Dial)) return 13;
    const auto c = mailbox.begin_session();
    if (c <= b || !mailbox.enqueue({BaresipEventKind::Registering, c})) return 14;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.sip != SipStatus::Registering) return 15;
    guard.revoke(RuntimeProcess::Stopped, mailbox, projection);
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, c})) return 16;
    const auto early = mailbox.begin_session();
    if (!mailbox.enqueue({BaresipEventKind::Registering, early})) return 17;
    state = guard.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox, projection);
    if (state.session || state.sip != SipStatus::Unavailable) return 18;
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, early})) return 19;
    return 0;
}
