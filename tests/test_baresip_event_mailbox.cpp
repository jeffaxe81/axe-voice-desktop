#include "../src/baresip_event_mailbox.h"
#include <atomic>
#include <thread>

int main() {
    using namespace axv;
    BaresipEventMailbox mailbox;
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, 1})) return 1;
    if (mailbox.drain().size) return 2;
    const auto first = mailbox.begin_session();
    if (!first || !mailbox.enqueue({BaresipEventKind::Registering, first})) return 3;
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, first + 1})) return 4;
    const auto initial = mailbox.drain();
    if (initial.size != 1 || initial.session != first ||
        initial.events[0].kind != BaresipEventKind::Registering) return 5;
    if (mailbox.drain().size) return 6;
    std::atomic<bool> failed{false};
    std::thread worker([&] {
        for (int i = 0; i < 20; ++i)
            if (!mailbox.enqueue({BaresipEventKind::CallOutgoing, first})) failed = true;
    });
    worker.join();
    const auto concurrent = mailbox.drain();
    if (failed || concurrent.size != 20 || concurrent.overflow) return 7;
    for (std::size_t i = 0; i < BaresipEventMailbox::kCapacity; ++i)
        if (!mailbox.enqueue({BaresipEventKind::CallIncoming, first})) return 8;
    if (mailbox.enqueue({BaresipEventKind::CallEstablished, first})) return 9;
    const auto overflow = mailbox.drain();
    if (!overflow.overflow || overflow.size != 0) return 10;
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, first})) return 11;
    if (!mailbox.drain().overflow) return 12;
    mailbox.invalidate_session();
    if (mailbox.enqueue({BaresipEventKind::RegisterOk, first})) return 13;
    const auto second = mailbox.begin_session();
    if (second <= first || mailbox.enqueue({BaresipEventKind::RegisterOk, first})) return 14;
    if (!mailbox.enqueue({BaresipEventKind::Registering, second})) return 15;
    const auto restarted = mailbox.drain();
    if (restarted.overflow || restarted.size != 1 || restarted.events[0].session != second) return 16;
    return 0;
}
