#include "../src/baresip_native_ingress.h"
#include <cstdint>

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    BaresipNativeIngress gate(mailbox);
    constexpr std::uintptr_t account = 101, call = 201;
    if (gate.accept({NativeSignal::RegisterOk, 1, account, 0}) != NativeIngressResult::Rejected) return 1;
    if (gate.bind_validated_account(0)) return 2;
    const auto a = gate.bind_validated_account(account);
    if (!a || gate.bind_validated_account(account)) return 3;
    if (gate.accept({NativeSignal::Unrelated, a, account, 0}) != NativeIngressResult::Ignored) return 4;
    if (gate.accept({NativeSignal::Registering, a, account, 0}) != NativeIngressResult::Queued ||
        gate.accept({NativeSignal::RegisterOk, a, account, 0}) != NativeIngressResult::Queued) return 5;
    const auto batch = mailbox.drain();
    if (batch.session != a || batch.size != 2 ||
        batch.events[0].kind != BaresipEventKind::Registering ||
        batch.events[1].kind != BaresipEventKind::RegisterOk) return 6;
    if (gate.accept({NativeSignal::CallIncoming, a, account, call}) != NativeIngressResult::Queued ||
        gate.accept({NativeSignal::CallEstablished, a, account, call}) != NativeIngressResult::Queued ||
        gate.accept({NativeSignal::CallClosed, a, account, call}) != NativeIngressResult::Queued ||
        gate.accept({NativeSignal::CallOutgoing, a, account, call + 1}) != NativeIngressResult::Queued) return 7;
    const auto calls = mailbox.drain();
    if (calls.size != 4 ||
        calls.events[0].call_id != 1 || calls.events[1].call_id != 1 ||
        calls.events[2].call_id != 1 || calls.events[3].call_id != 2) return 8;
    if (gate.accept({NativeSignal::CallClosed, a, account, call}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::CallMismatch || mailbox.drain().size) return 9;
    if (gate.bind_validated_account(account)) return 9; // fault latched
    gate.reset_after_teardown();
    const auto b = gate.bind_validated_account(account);
    if (b <= a) return 10;
    if (gate.accept({NativeSignal::Registering, a, account, 0}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::SessionMismatch) return 11;
    gate.reset_after_teardown();
    const auto c = gate.bind_validated_account(account);
    if (gate.accept({NativeSignal::Registering, c, account + 1, 0}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::AccountMismatch) return 12;
    gate.reset_after_teardown();
    const auto d = gate.bind_validated_account(account);
    if (gate.accept({NativeSignal::CallOutgoing, d, account, 0}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::CallMismatch) return 13;
    gate.reset_after_teardown();
    const auto e = gate.bind_validated_account(account);
    if (gate.accept({NativeSignal::Registering, e, account, 7}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::Malformed) return 14;
    gate.reset_after_teardown();
    const auto f = gate.bind_validated_account(account);
    if (gate.accept({static_cast<NativeSignal>(999), f, account, 0}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::Unrecognized) return 15;
    gate.reset_after_teardown();
    const auto g = gate.bind_validated_account(account);
    for (std::size_t i = 0; i < BaresipEventMailbox::kCapacity; ++i) {
        if (gate.accept({NativeSignal::Registering, g, account, 0}) != NativeIngressResult::Queued) return 16;
    }
    if (gate.accept({NativeSignal::RegisterOk, g, account, 0}) != NativeIngressResult::Revoked ||
        gate.fault() != NativeIngressFault::QueueRejected || mailbox.drain().size) return 17;
    gate.reset_after_teardown();
    if (gate.session() != 0 || gate.fault() != NativeIngressFault::None ||
        gate.accept({NativeSignal::RegisterOk, g, account, 0}) != NativeIngressResult::Rejected) return 18;
    return 0;
}
