#include "incoming_ringtone_worker.h"
#include "audio_preview.h"
#include <chrono>
#include <utility>

namespace axv {
CancellableIncomingRingtoneSink::CancellableIncomingRingtoneSink(
    Render render, std::uint8_t level)
    : render_(std::move(render)), level_(level) {}
CancellableIncomingRingtoneSink::~CancellableIncomingRingtoneSink() {
    cancel_.store(true, std::memory_order_release);
    wake_.notify_all();
    if (worker_.joinable()) worker_.join();
}
bool CancellableIncomingRingtoneSink::start(const std::wstring& endpoint) {
    if (endpoint.empty() || level_>100 || !render_ || worker_.joinable() ||
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
        while (!cancel_.load(std::memory_order_acquire)) {
            if (!render_(endpoint,level_)) {
                failed_.store(true,std::memory_order_release);
                break;
            }
            std::unique_lock<std::mutex> lk(mutex_);
            wake_.wait_for(lk,std::chrono::milliseconds(1100),
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
bool windows_incoming_ring_tone_once(const std::wstring& id,std::uint8_t level) {
    return play_preview_tone_on_endpoint(id,level)==AudioPreviewResult::Completed;
}
} // namespace axv
