#include "../src/baresip_native_ingress.h"
#include "../src/sip_event_projection.h"
#include <atomic>
#include <thread>

int main() {
    using namespace axv;
    constexpr std::uintptr_t account = 0x101, call = 0x201;
    BaresipEventMailbox mailbox;
    BaresipNativeIngress ingress(mailbox);
    SipEventProjection projection;

    // An event-producing contract is NOT itself a trusted native adapter.
    const auto untrusted = ingress.bind_validated_account(account);
    if (!untrusted) return 1;
    if (ingress.accept({NativeSignal::Registering, untrusted, account, 0}) != NativeIngressResult::Queued ||
        ingress.accept({NativeSignal::RegisterOk, untrusted, account, 0}) != NativeIngressResult::Queued)
        return 2;
    auto state = projection.observe(RuntimeProcess::Running, SipEvidence::None, mailbox.drain());
    if (state.evidence != SipEvidence::None ||
        may_use_call_control(state, untrusted, CallAction::Dial)) return 3;
    ingress.reset_after_teardown();

    // VerifiedNativeApi below is SYNTHETIC TEST INPUT ONLY. Production never
    // asserts it without an audited in-process Baresip integration.
    const auto session = ingress.bind_validated_account(account);
    if (session <= untrusted) return 4;
    if (ingress.accept({NativeSignal::Registering, session, account, 0}) != NativeIngressResult::Queued ||
        ingress.accept({NativeSignal::RegisterOk, session, account, 0}) != NativeIngressResult::Queued)
        return 5;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered ||
        !may_use_call_control(state, session, CallAction::Dial)) return 6;
    if (ingress.accept({NativeSignal::CallIncoming, session, account, call}) != NativeIngressResult::Queued)
        return 7;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Ringing ||
        !may_use_call_control(state, session, CallAction::Answer) || state.call_id != 1)
        return 8;
    if (ingress.accept({NativeSignal::CallEstablished, session, account, call}) != NativeIngressResult::Queued)
        return 9;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::InCall ||
        !may_use_call_control(state, session, CallAction::Hangup) ||
        may_use_call_control(state, session, CallAction::Dial)) return 10;
    if (ingress.accept({NativeSignal::CallClosed, session, account, call}) != NativeIngressResult::Queued)
        return 11;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.sip != SipStatus::Registered || state.call_id != 0 || state.last_call_id != 1)
        return 12;

    // A mismatched account revokes the ENTIRE queued history, not just the
    // offending callback. A later synthetic "verified" claim cannot revive it.
    if (ingress.accept({NativeSignal::CallOutgoing, session, account, call + 1}) != NativeIngressResult::Queued ||
        ingress.accept({NativeSignal::RegisterOk, session, account + 1, 0}) != NativeIngressResult::Revoked)
        return 13;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.evidence != SipEvidence::None ||
        may_use_call_control(state, session, CallAction::Dial) || ingress.session() != 0)
        return 14;

    ingress.reset_after_teardown();
    const auto malformedSession = ingress.bind_validated_account(account);
    if (malformedSession <= session ||
        ingress.accept({NativeSignal::Registering, malformedSession, account, 0}) != NativeIngressResult::Queued)
        return 15;
    auto malformed = mailbox.drain();
    malformed.size = BaresipEventMailbox::kCapacity + 1; // impossible from a real drain
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, malformed);
    if (state.evidence != SipEvidence::None || state.session != 0) return 16;
    if (ingress.accept({NativeSignal::RegisterOk, malformedSession, account, 0}) != NativeIngressResult::Queued)
        return 17;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (state.evidence != SipEvidence::None) return 18; // generation permanently rejected

    ingress.reset_after_teardown();
    const auto fresh = ingress.bind_validated_account(account);
    if (fresh <= malformedSession) return 19;
    if (ingress.accept({NativeSignal::Registering, fresh, account, 0}) != NativeIngressResult::Queued ||
        ingress.accept({NativeSignal::RegisterOk, fresh, account, 0}) != NativeIngressResult::Queued)
        return 20;
    state = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (!may_use_call_control(state, fresh, CallAction::Dial)) return 21;

    // Teardown races with callbacks: late messages must never be admitted to
    // a new generation, and no already queued event may survive teardown.
    std::atomic<bool> go{false};
    std::atomic<bool> unexpected{false};
    std::thread callback([&] {
        while (!go.load()) std::this_thread::yield();
        for (int i = 0; i < 1000; ++i) {
            const auto result = ingress.accept({NativeSignal::Unrelated, fresh, account, 0});
            if (result != NativeIngressResult::Ignored && result != NativeIngressResult::Rejected)
                unexpected = true;
        }
    });
    go = true;
    ingress.reset_after_teardown();
    callback.join();
    if (unexpected || ingress.session() != 0 || mailbox.drain().size != 0)
        return 22;
    state = projection.observe(RuntimeProcess::Stopped, SipEvidence::None, mailbox.drain());
    if (state.evidence != SipEvidence::None || state.sip != SipStatus::Unavailable)
        return 23;
    return 0;
}
