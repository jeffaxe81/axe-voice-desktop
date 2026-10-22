#include "../src/baresip_callback_ingress.h"
#include "../src/sip_event_projection.h"
#include <atomic>
#include <thread>

int main() {
    using namespace axv;
    constexpr std::uintptr_t account = 0xA1;
    BaresipEventMailbox mailbox;
    BaresipCallbackIngress ingress(mailbox);
    SipEventProjection projection;

    // An admitted callback stream does not independently establish provenance.
    const auto untrusted = ingress.begin_session(account);
    if (!untrusted ||
        !ingress.registration(untrusted, account, BaresipEventKind::Registering) ||
        !ingress.registration(untrusted, account, BaresipEventKind::RegisterOk))
        return 1;
    auto state = projection.observe(RuntimeProcess::Running,
                                    SipEvidence::None, mailbox.drain());
    if (state.evidence != SipEvidence::None ||
        may_use_call_control(state, untrusted, CallAction::Dial)) return 2;
    ingress.revoke();

    // In this UNIT TEST ONLY the synthetic proof below exercises the reducer.
    // Neither the ingress nor the production UI can grant VerifiedNativeApi.
    const auto session = ingress.begin_session(account);
    if (session <= untrusted ||
        !ingress.registration(session, account, BaresipEventKind::Registering) ||
        !ingress.registration(session, account, BaresipEventKind::RegisterOk))
        return 3;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered ||
        !may_use_call_control(state, session, CallAction::Dial)) return 4;

    // A call identity remains stable while its UI state progresses.
    if (!ingress.call_started(session, account, 101, BaresipEventKind::CallIncoming))
        return 5;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Ringing || state.call_id != 1 ||
        !may_use_call_control(state, session, CallAction::Answer)) return 6;
    if (!ingress.call_changed(session, account, 101, BaresipEventKind::CallEstablished) ||
        !ingress.call_changed(session, account, 101, BaresipEventKind::CallClosed))
        return 7;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered || state.call_id != 0 ||
        state.last_call_id != 1) return 8;

    // The mailbox has fixed storage; a fabricated oversized batch MUST be
    // rejected without reading beyond the array or resurrecting the session.
    if (!ingress.call_started(session, account, 102, BaresipEventKind::CallOutgoing))
        return 9;
    auto malformed = mailbox.drain();
    malformed.size = BaresipEventMailbox::kCapacity + 1;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, malformed);
    if (state.evidence != SipEvidence::None || state.session != 0 ||
        may_use_call_control(state, session, CallAction::Hangup)) return 10;
    if (!ingress.call_changed(session, account, 102, BaresipEventKind::CallEstablished))
        return 11;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.evidence != SipEvidence::None) return 12;

    ingress.revoke();
    const auto fresh = ingress.begin_session(account);
    if (fresh <= session || ingress.registration(session, account, BaresipEventKind::RegisterOk))
        return 13;
    if (!ingress.registration(fresh, account, BaresipEventKind::Registering) ||
        !ingress.registration(fresh, account, BaresipEventKind::RegisterOk))
        return 14;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered) return 15;

    // Invalid event class revokes and erases ALL queued data, without
    // implicitly granting the next generation.
    if (!ingress.registration(fresh, account, BaresipEventKind::Registering))
        return 16;
    if (ingress.registration(fresh, account, BaresipEventKind::CallIncoming) ||
        ingress.session() != 0 || mailbox.drain().session != 0)
        return 17;
    state = projection.observe(RuntimeProcess::Running,
                               SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.evidence != SipEvidence::None ||
        may_use_call_control(state, fresh, CallAction::Dial)) return 18;

    // Revoke is mutex-synchronized with in-flight worker callbacks; even
    // if some callbacks race ahead, no history survives a completed teardown.
    const auto last = ingress.begin_session(account);
    if (last <= fresh) return 19;
    std::atomic<bool> start{false};
    std::thread worker([&] {
        while (!start.load()) std::this_thread::yield();
        for (int i = 0; i < 200; ++i)
            ingress.registration(last, account, BaresipEventKind::Registering);
    });
    start = true;
    ingress.revoke();
    worker.join();
    if (ingress.session() != 0 || mailbox.drain().session != 0 ||
        ingress.registration(last, account, BaresipEventKind::RegisterOk)) return 20;
    const auto mismatched = ingress.begin_session(account);
    if (mismatched <= last ||
        ingress.registration(mismatched, account + 1, BaresipEventKind::Registering) ||
        ingress.session() != 0 || mailbox.drain().session != 0) return 21;
    return 0;
}
