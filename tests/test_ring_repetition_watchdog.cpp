#include "../src/incoming_ringtone_worker.h"
#include <atomic>
#include <chrono>
#include <thread>

int main() {
    using namespace axv;
    using namespace std::chrono_literals;
    std::atomic<unsigned> played{0};
    CancellableIncomingRingtoneSink sink(
        [&](const std::wstring&, std::uint8_t, const std::atomic<bool>&) {
            played.fetch_add(1, std::memory_order_relaxed);
            return true;
        }, 75, 3, 10ms);
    if (!sink.start(L"headset")) return 1;
    for (int i=0;i<200 && !sink.finished();++i) std::this_thread::sleep_for(5ms);
    if (!sink.finished() || sink.operational() || played.load()!=3) return 2;
    if (!sink.stop()) return 3;
    // Failed completion stays fail-closed until a NEW explicitly requested start.
    if (!sink.start(L"pc")) return 4;
    for (int i=0;i<200 && !sink.finished();++i) std::this_thread::sleep_for(5ms);
    if (!sink.finished() || sink.operational() || played.load()!=6) return 5;
    if (!sink.stop()) return 6;
    CancellableIncomingRingtoneSink no_cycles(
        [](const std::wstring&, std::uint8_t, const std::atomic<bool>&){ return true; },
        75, 0, 10ms);
    if (no_cycles.start(L"pc")) return 7;
    CancellableIncomingRingtoneSink too_many(
        [](const std::wstring&, std::uint8_t, const std::atomic<bool>&){ return true; },
        75, 61, 10ms);
    if (too_many.start(L"pc")) return 8;
    CancellableIncomingRingtoneSink too_fast(
        [](const std::wstring&, std::uint8_t, const std::atomic<bool>&){ return true; },
        75, 3, 0ms);
    if (too_fast.start(L"pc")) return 9;
    CancellableIncomingRingtoneSink too_slow(
        [](const std::wstring&, std::uint8_t, const std::atomic<bool>&){ return true; },
        75, 3, 6000ms);
    if (too_slow.start(L"pc")) return 10;
    return 0;
}
