#include "incoming_ringtone_worker.h"
#include "ringtone_preview.h"
#include <chrono>
#include <utility>

namespace axv {
CancellableIncomingRingtoneSink::CancellableIncomingRingtoneSink(
    Render render, std::uint8_t level, std::uint32_t max_cycles,
    std::chrono::milliseconds quiet_interval)
    : render_(std::move(render)), level_(level), max_cycles_(max_cycles),
      quiet_interval_(quiet_interval) {}
CancellableIncomingRingtoneSink::~CancellableIncomingRingtoneSink() {
    cancel_.store(true, std::memory_order_release);
    wake_.notify_all();
    if (worker_.joinable()) worker_.join();
}
bool CancellableIncomingRingtoneSink::start(const std::wstring& endpoint) {
    if (endpoint.empty() || level_>100 || max_cycles_==0 || max_cycles_>60 ||
        quiet_interval_<std::chrono::milliseconds(10) ||
        quiet_interval_>std::chrono::milliseconds(5000) ||
        !render_ || worker_.joinable() ||
        !finished()) return false;
    cancel_.store(false, std::memory_order_release);
    failed_.store(false, std::memory_order_release);
    finished_.store(false, std::memory_order_release);
    try {
        worker_=std::thread(&CancellableIncomingRingtoneSink::loop,this,endpoint);
    } catch (...) {
        failed_.store(true, std::memory_order_release);
        finished_.store(true, std::memory_order_release);
        return false;
    }
    return true;
}
void CancellableIncomingRingtoneSink::loop(std::wstring endpoint) noexcept {
    try {
        std::uint32_t cycles=0;
        while (!cancel_.load(std::memory_order_acquire)) {
            // Bounded safety watchdog: do not ring indefinitely on stale SIP state.
            if (cycles >= max_cycles_) {
                failed_.store(true,std::memory_order_release);
                break;
            }
            ++cycles;
            if (!render_(endpoint,level_,cancel_)) {
                failed_.store(true,std::memory_order_release);
                break;
            }
            std::unique_lock<std::mutex> lk(mutex_);
            wake_.wait_for(lk,quiet_interval_,
                [this]{return cancel_.load(std::memory_order_acquire);});
        }
    } catch (...) {
        failed_.store(true,std::memory_order_release);
    }
    finished_.store(true,std::memory_order_release);
}
bool CancellableIncomingRingtoneSink::stop() {
    cancel_.store(true,std::memory_order_release);
    wake_.notify_all();
    if (!finished()) return false;
    if (worker_.joinable()) worker_.join();
    return true;
}
bool CancellableIncomingRingtoneSink::operational() const {
    return !failed_.load(std::memory_order_acquire);
}
bool windows_incoming_ring_tone_once(const std::wstring& id,std::uint8_t level,
                                    const std::atomic<bool>& stop) {
    return play_selected_ringtone_preview(id, &stop, level)==RingPreviewResult::Played;
}
} // namespace axv
