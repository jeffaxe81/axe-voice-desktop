#include "../src/baresip_callback_lifecycle.h"
#include "../src/sip_event_projection.h"
#include <atomic>
#include <thread>
#include <vector>

int main() {
    using namespace axv;
    constexpr std::uintptr_t account = 0xA1;

    // Fence: closing rejects new work while a previously admitted callback
    // still owns a lease. Waiting allows a new generation only after release.
    BaresipCallbackFence fence;
    const auto generation = fence.begin();
    if (!generation || fence.begin()) return 1;
    auto lease = fence.admit(generation);
    if (!lease || fence.admit(generation - 1)) return 2;
    fence.close();
    if (fence.admit(generation) || fence.begin()) return 3;
    std::atomic<bool> completed{false};
    std::thread waiter([&] {
        fence.wait();
        completed.store(true);
    });
    lease.release();
    waiter.join();
    if (!completed || !fence.begin()) return 4;
    if (fence.admit(generation)) return 5;
    fence.close_and_wait();

    BaresipEventMailbox mailbox;
    BaresipCallbackLifecycle lifecycle(mailbox);
    const auto first = lifecycle.begin(account);
    if (!first || lifecycle.begin(account)) return 6;
    if (!lifecycle.registration(first, BaresipEventKind::Registering) ||
        !lifecycle.registration(first, BaresipEventKind::RegisterOk) ||
        !lifecycle.call_started(first, 200, BaresipEventKind::CallIncoming) ||
        !lifecycle.call_changed(first, 200, BaresipEventKind::CallEstablished) ||
        !lifecycle.call_changed(first, 200, BaresipEventKind::CallClosed)) return 7;
    const auto batch = mailbox.drain();
    if (batch.session != first.mailbox_session || batch.size != 5 ||
        batch.events[2].call_id != 1 || batch.events[4].call_id != 1) return 8;
    SipEventProjection projection;
    // Synthetic verified evidence is TEST ONLY and never issued by lifecycle.
    const auto observed = projection.observe(RuntimeProcess::Running,
                                               SipEvidence::VerifiedNativeApi, batch);
    if (observed.sip != SipStatus::Registered || observed.call_id) return 9;

    lifecycle.revoke_and_wait();
    if (lifecycle.registration(first, BaresipEventKind::Registering) ||
        lifecycle.call_started(first, 201, BaresipEventKind::CallOutgoing) ||
        mailbox.drain().session) return 10;
    const auto second = lifecycle.begin(account);
    if (!second || second.fence_generation <= first.fence_generation ||
        second.mailbox_session <= first.mailbox_session) return 11;

    std::atomic<bool> stale_accepted{false};
    std::vector<std::thread> late_callbacks;
    for (int n = 0; n < 12; ++n) {
        late_callbacks.emplace_back([&] {
            if (lifecycle.registration(first, BaresipEventKind::RegisterOk) ||
                lifecycle.call_changed(first, 200, BaresipEventKind::CallClosed) ||
                lifecycle.call_started(first, 202, BaresipEventKind::CallIncoming))
                stale_accepted = true;
        });
    }
    for (auto& t : late_callbacks) t.join();
    if (stale_accepted) return 12;
    if (!lifecycle.registration(second, BaresipEventKind::Registering) ||
        !lifecycle.registration(second, BaresipEventKind::RegisterOk)) return 13;
    const auto next_batch = mailbox.drain();
    if (next_batch.session != second.mailbox_session || next_batch.size != 2) return 14;

    // A wrong call token revokes the current mailbox but must not cause the
    // lifecycle boundary to silently reopen a new generation.
    if (!lifecycle.call_started(second, 5, BaresipEventKind::CallOutgoing)) return 15;
    if (lifecycle.call_changed(second, 6, BaresipEventKind::CallClosed)) return 16;
    if (lifecycle.begin(account) || mailbox.drain().session) return 17;
    lifecycle.revoke_and_wait();
    const auto third = lifecycle.begin(account);
    if (!third || third.fence_generation <= second.fence_generation) return 18;
    if (lifecycle.registration(second, BaresipEventKind::Registering)) return 19;
    if (!lifecycle.registration(third, BaresipEventKind::Registering)) return 20;
    // A forged account on a current generation clears its mailbox evidence.
    auto wrong = third;
    wrong.account_identity = account + 1;
    if (lifecycle.registration(wrong, BaresipEventKind::RegisterOk) ||
        mailbox.drain().session != 0) return 21;
    lifecycle.revoke_and_wait();
    return 0;
}
