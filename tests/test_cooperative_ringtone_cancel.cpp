#include "../src/incoming_ringtone_worker.h"
#include <atomic>
#include <chrono>
#include <thread>

int main() {
    using namespace axv;
    using namespace std::chrono_literals;
    std::atomic<int> started{0};
    std::atomic<int> saw_cancel{0};
    CancellableIncomingRingtoneSink sink(
        [&](const std::wstring&, std::uint8_t, const std::atomic<bool>& stop) {
            started.fetch_add(1);
            // Synthetic render loop honors the SAME cancellation signal WASAPI gets.
            for (int i=0;i<200 && !stop.load(std::memory_order_acquire);++i)
                std::this_thread::sleep_for(5ms);
            if (stop.load(std::memory_order_acquire)) saw_cancel.fetch_add(1);
            return true;
        });
    if (!sink.start(L"headset")) return 1;
    for(int i=0;i<100 && started.load()==0;++i) std::this_thread::sleep_for(5ms);
    if (started.load()!=1 || sink.finished()) return 2;
    bool stopped=sink.stop();
    for(int i=0;i<200 && !stopped;++i) {
        std::this_thread::sleep_for(5ms);
        stopped=sink.stop();
    }
    if(!stopped || saw_cancel.load()!=1 || !sink.finished()) return 3;
    if(!sink.start(L"speaker")) return 4;
    for(int i=0;i<100 && started.load()<2;++i) std::this_thread::sleep_for(5ms);
    if(started.load()!=2) return 5;
    stopped=sink.stop();
    for(int i=0;i<200 && !stopped;++i) {
        std::this_thread::sleep_for(5ms);
        stopped=sink.stop();
    }
    if(!stopped || saw_cancel.load()!=2 || !sink.finished()) return 6;
    return 0;
}
