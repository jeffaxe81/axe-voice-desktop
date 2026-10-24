#include "../src/baresip_callback_ingress.h"
#include "../src/sip_event_projection.h"
#include <atomic>
#include <thread>
#include <vector>

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipCallbackIngress ingress(mailbox);
    SipEventProjection projection;
    if (ingress.registration(1, BaresipEventKind::RegisterOk)) return 1;
    const auto a = ingress.begin_session();
    if (!a || ingress.begin_session() != 0) return 2;
    if (!ingress.registration(a, BaresipEventKind::Registering) ||
        !ingress.registration(a, BaresipEventKind::RegisterOk)) return 3;
    // Test-supplied synthetic proof only. This class never grants trust.
    auto observed = projection.observe(RuntimeProcess::Running,
                                      SipEvidence::VerifiedNativeApi, mailbox.drain());
    if (observed.sip != SipStatus::Registered) return 4;
    if (!ingress.call_started(a, 800, BaresipEventKind::CallOutgoing) ||
        !ingress.call_changed(a, 800, BaresipEventKind::CallEstablished) ||
        !ingress.call_changed(a, 800, BaresipEventKind::CallClosed) ||
        !ingress.call_started(a, 900, BaresipEventKind::CallIncoming)) return 5;
    const auto good = mailbox.drain();
    if (good.size != 4 ||
        good.events[0].call_id != 1 || good.events[1].call_id != 1 ||
        good.events[2].call_id != 1 || good.events[3].call_id != 2) return 6;
    observed = projection.observe(RuntimeProcess::Running, SipEvidence::VerifiedNativeApi, good);
    if (observed.sip != SipStatus::Ringing || observed.call_id != 2) return 7;

    // Late close from a prior call revokes rather than hanging up the new call.
    if (ingress.call_changed(a, 800, BaresipEventKind::CallClosed)) return 8;
    if (ingress.session() || mailbox.drain().session) return 9;
    const auto b = ingress.begin_session();
    if (b <= a) return 10;
    // Concurrent callbacks from a retired generation cannot poison current one.
    std::atomic<bool> accepted_stale{false};
    std::vector<std::thread> old_workers;
    for (int i = 0; i < 8; ++i) {
        old_workers.emplace_back([&] {
            if (ingress.registration(a, BaresipEventKind::RegisterOk) ||
                ingress.call_started(a, 1001, BaresipEventKind::CallIncoming))
                accepted_stale = true;
        });
    }
    for (auto& worker : old_workers) worker.join();
    if (accepted_stale || ingress.session() != b) return 11;
    if (!ingress.registration(b, BaresipEventKind::Registering) ||
        !ingress.registration(b, BaresipEventKind::RegisterOk)) return 12;
    auto fresh = mailbox.drain();
    if (fresh.session != b || fresh.size != 2 || fresh.overflow) return 13;
    // Reuse of a native call key within the same generation is forbidden.
    if (!ingress.call_started(b, 7, BaresipEventKind::CallOutgoing) ||
        !ingress.call_changed(b, 7, BaresipEventKind::CallClosed)) return 14;
    if (ingress.call_started(b, 7, BaresipEventKind::CallOutgoing)) return 15;
    if (ingress.session()) return 16;
    const auto c = ingress.begin_session();
    if (c <= b || !ingress.registration(c, BaresipEventKind::TransportLost)) return 17;
    if (mailbox.drain().events[0].kind != BaresipEventKind::TransportLost) return 18;
    // More than 32 undrained callbacks causes irrevocable session fencing.
    for (std::size_t i = 0; i < BaresipEventMailbox::kCapacity; ++i)
        if (!ingress.registration(c, BaresipEventKind::Registering)) return 19;
    if (ingress.registration(c, BaresipEventKind::RegisterOk)) return 20;
    if (ingress.session() || mailbox.drain().size) return 21;
    const auto d = ingress.begin_session();
    if (d <= c) return 22;
    ingress.revoke();
    if (ingress.call_started(d, 1, BaresipEventKind::CallIncoming)) return 23;
    if (ingress.session() != 0) return 24;
    return 0;
}
