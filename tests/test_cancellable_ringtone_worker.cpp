#include "../src/incoming_ringtone_worker.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>

int main() {
    using namespace axv;
    using namespace std::chrono_literals;
    std::mutex mu;
    std::condition_variable cv;
    bool entered=false, release=false;
    int calls=0;
    std::wstring device;
    std::uint8_t level=0;
    CancellableIncomingRingtoneSink sink(
        [&](const std::wstring& id,std::uint8_t gain) {
            std::unique_lock<std::mutex> lock(mu);
            device=id;level=gain;++calls;entered=true;cv.notify_all();
            return cv.wait_for(lock,1500ms,[&]{return release;});
        },50);
    if(!sink.finished() || !sink.operational() || sink.start(L"")) return 1;
    if(!sink.start(L"pc") || sink.start(L"headset")) return 2;
    {
        std::unique_lock<std::mutex> lock(mu);
        if(!cv.wait_for(lock,1200ms,[&]{return entered;})) return 3;
        if(calls!=1 || device!=L"pc" || level!=50) return 4;
    }
    if(sink.stop() || sink.finished()) return 5;
    { std::lock_guard<std::mutex> lock(mu);release=true; }
    cv.notify_all();
    bool stopped=false;
    for(int i=0;i<100 && !(stopped=sink.stop());++i)
        std::this_thread::sleep_for(10ms);
    if(!stopped || !sink.finished() || !sink.operational()) return 6;
    if(!sink.start(L"headset")) return 7;
    {
        std::unique_lock<std::mutex> lock(mu);
        if(!cv.wait_for(lock,1200ms,[&]{return calls>=2;})) return 8;
        if(device!=L"headset") return 9;
    }
    for(int i=0;i<100 && !sink.stop();++i)
        std::this_thread::sleep_for(10ms);
    if(!sink.stop()) return 10;
    CancellableIncomingRingtoneSink error(
        [](const std::wstring&,std::uint8_t){return false;});
    if(!error.start(L"pc")) return 11;
    for(int i=0;i<100 && !error.finished();++i)
        std::this_thread::sleep_for(10ms);
    if(!error.finished() || error.operational() || !error.stop()) return 12;
    CancellableIncomingRingtoneSink invalid(
        [](const std::wstring&,std::uint8_t){return true;},101);
    if(invalid.start(L"pc") || !invalid.stop()) return 13;
    return 0;
}
