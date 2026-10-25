#include "../src/sip_event_projection.h"

int main() {
    using namespace axv;
    SipEventProjection projection;
    BaresipEventMailbox mailbox;
    auto state = projection.observe(RuntimeProcess::Running, SipEvidence::None, mailbox.drain());
    if (state.sip != SipStatus::Unavailable) return 1;
    auto first = mailbox.begin_session();
    mailbox.enqueue({BaresipEventKind::Registering, first,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::None, mailbox.drain());
    if (state.evidence != SipEvidence::None || state.session != 0) return 2;
    // Synthetic proof models ONLY the contract a future audited native adapter must meet.
    mailbox.enqueue({BaresipEventKind::Registering, first,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.sip != SipStatus::Registering || state.session != first) return 3;
    mailbox.enqueue({BaresipEventKind::RegisterOk, first,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (!may_use_call_control(state, first, CallAction::Dial)) return 4;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.sip != SipStatus::Registered) return 5; // persist across UI timer ticks
    state = projection.observe(RuntimeProcess::Running, SipEvidence::None, mailbox.drain());
    if (may_use_call_control(state, first, CallAction::Dial)) return 6;
    mailbox.enqueue({BaresipEventKind::Registering, first,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.evidence != SipEvidence::None) return 7; // proof cannot revive same generation
    auto second = mailbox.begin_session();
    mailbox.enqueue({BaresipEventKind::Registering, second,0,77,0});
    mailbox.enqueue({BaresipEventKind::RegisterOk, second,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (!may_use_call_control(state, second, CallAction::Dial)) return 8;
    mailbox.enqueue({BaresipEventKind::CallOutgoing, second,0,77,201});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.sip != SipStatus::Calling) return 9;
    mailbox.enqueue({BaresipEventKind::RegisterOk, second,0,77,0}); // invalid while Calling
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.sip != SipStatus::Unavailable || state.session != 0) return 10;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.evidence != SipEvidence::None) return 11;
    auto third = mailbox.begin_session();
    for (std::size_t i = 0; i < BaresipEventMailbox::kCapacity; ++i)
        if (!mailbox.enqueue({BaresipEventKind::Registering, third,0,77,0})) return 12;
    mailbox.enqueue({BaresipEventKind::RegisterOk, third,0,77,0}); // overflow discards all events
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.evidence != SipEvidence::None) return 13;
    auto fourth = mailbox.begin_session();
    mailbox.enqueue({BaresipEventKind::Registering, fourth,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.sip != SipStatus::Registering) return 14;
    state = projection.observe(RuntimeProcess::Stopping, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.evidence != SipEvidence::None || state.session != 0) return 15;
    mailbox.enqueue({BaresipEventKind::RegisterOk, fourth,0,77,0});
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain(), 77);
    if (state.evidence != SipEvidence::None) return 16;
    mailbox.invalidate_session();
    state = projection.observe(RuntimeProcess::Stopped, SipEvidence::None, mailbox.drain());
    if (state.process != RuntimeProcess::Stopped || state.sip != SipStatus::Unavailable) return 17;
    // A proof source cannot silently switch accounts within the same session.
    auto fifth = mailbox.begin_session();
    mailbox.enqueue({BaresipEventKind::Registering,fifth,0,77,0});
    state = projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain(),77);
    if (state.sip != SipStatus::Registering) return 18;
    state = projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain(),78);
    if (state.evidence != SipEvidence::None || state.session != 0) return 19;
    mailbox.enqueue({BaresipEventKind::RegisterOk,fifth,0,77,0});
    state = projection.observe(RuntimeProcess::Running,SipEvidence::VerifiedNativeApi,mailbox.drain(),77);
    if (state.evidence != SipEvidence::None) return 20;
    return 0;
}
