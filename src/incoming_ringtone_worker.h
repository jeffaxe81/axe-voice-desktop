#pragma once
#include "incoming_ringtone_coordinator.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace axv {
// Future ringtone sink only. Main executable uses a disabled sink until verified
// SIP callbacks are integrated. Stop returns true only after the render thread
// has terminated; no detached worker or concurrent dual-output playback.
class CancellableIncomingRingtoneSink final : public IncomingRingtoneSink {
public:
    using Render = std::function<bool(const std::wstring&, std::uint8_t, const std::atomic<bool>&)>;
    explicit CancellableIncomingRingtoneSink(Render render, std::uint8_t level=75);
    ~CancellableIncomingRingtoneSink() override;
    CancellableIncomingRingtoneSink(const CancellableIncomingRingtoneSink&)=delete;
    CancellableIncomingRingtoneSink& operator=(const CancellableIncomingRingtoneSink&)=delete;
    bool start(const std::wstring& endpoint) override;
    bool stop() override;
    bool operational() const override;
    bool finished() const { return finished_.load(std::memory_order_acquire); }
private:
    void loop(std::wstring endpoint) noexcept;
    Render render_;
    std::uint8_t level_;
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::atomic<bool> cancel_{false};
    std::atomic<bool> finished_{true};
    std::atomic<bool> failed_{false};
};
// Short WASAPI audio test, not a SIP callback or registered call.
bool windows_incoming_ring_tone_once(const std::wstring&, std::uint8_t,
                                   const std::atomic<bool>&);
} // namespace axv
