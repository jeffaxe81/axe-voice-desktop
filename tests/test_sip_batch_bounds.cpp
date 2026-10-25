#include "../src/sip_event_projection.h"
#include <cstdint>
#include <limits>

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    SipEventProjection projection;
    const auto first = mailbox.begin_session();
    if (!first || !mailbox.enqueue({BaresipEventKind::Registering, first}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, first})) return 1;
    auto state = projection.observe(RuntimeProcess::Running,
                                    SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered ||
        !may_use_call_control(state, first, CallAction::Dial)) return 2;

    // Malformed batch created by a faulty/untrusted producer must never read
    // beyond the 32-event array, even if its first events look trustworthy.
    auto corrupt = mailbox.drain();
    corrupt.size = BaresipEventMailbox::kCapacity + 1;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, corrupt);
    if (state.evidence != SipEvidence::None || state.session != 0 ||
        may_use_call_control(state, first, CallAction::Dial)) return 3;

    // Reject recovery attempts within the same mailbox generation.
    if (!mailbox.enqueue({BaresipEventKind::Registering, first}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, first})) return 4;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.evidence != SipEvidence::None || state.session != 0) return 5;

    // A truly new generation can progress independently.
    const auto second = mailbox.begin_session();
    if (second <= first ||
        !mailbox.enqueue({BaresipEventKind::Registering, second}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, second})) return 6;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered ||
        !may_use_call_control(state, second, CallAction::Dial)) return 7;

    // Also reject an extreme length, without allocating or iterating it.
    corrupt = mailbox.drain();
    corrupt.size = std::numeric_limits<std::size_t>::max();
    corrupt.session = second;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, corrupt);
    if (state.evidence != SipEvidence::None ||
        may_use_call_control(state, second, CallAction::Dial)) return 8;
    const auto third = mailbox.begin_session();
    if (third <= second ||
        !mailbox.enqueue({BaresipEventKind::Registering, third}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, third})) return 9;

    // Even a valid-looking batch cannot grant trust without native proof.
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::None, mailbox.drain());
    if (state.evidence != SipEvidence::None ||
        may_use_call_control(state, third, CallAction::Dial)) return 10;
    // Extra LAB regression: a late batch from a previous generation must
    // revoke the newer live session, and cannot revive either session.
    const auto fourth = mailbox.begin_session();
    if (fourth <= third ||
        !mailbox.enqueue({BaresipEventKind::Registering, fourth}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, fourth})) return 11;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered ||
        !may_use_call_control(state, fourth, CallAction::Dial)) return 12;
    auto stale = mailbox.drain();
    stale.session = third;
    stale.size = 0;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, stale);
    if (state.session || state.evidence != SipEvidence::None ||
        may_use_call_control(state, fourth, CallAction::Dial)) return 13;
    if (!mailbox.enqueue({BaresipEventKind::Registering, fourth})) return 14;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.session || state.evidence != SipEvidence::None) return 15;
    const auto fifth = mailbox.begin_session();
    if (fifth <= fourth ||
        !mailbox.enqueue({BaresipEventKind::Registering, fifth}) ||
        !mailbox.enqueue({BaresipEventKind::RegisterOk, fifth})) return 16;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered ||
        !may_use_call_control(state, fifth, CallAction::Dial)) return 17;
    return 0;
}
