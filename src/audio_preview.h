#pragma once
#ifndef _WIN32
#error AXE Voice preview audio is Windows-only
#endif
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace axv {
// A local WASAPI test sound. Does NOT produce call ringing or SIP media.
enum class AudioPreviewResult {
    NotRun, Completed, NoEndpoint, WindowsAudioUnavailable,
    UnsupportedFormat, PlaybackFailed
};
AudioPreviewResult play_preview_tone_on_endpoint(
    const std::wstring& endpoint_id, std::uint8_t level_percent = 100);

class AudioPreviewWorker final {
public:
    AudioPreviewWorker() = default;
    AudioPreviewWorker(const AudioPreviewWorker&) = delete;
    AudioPreviewWorker& operator=(const AudioPreviewWorker&) = delete;
    ~AudioPreviewWorker() { wait(); }
    bool start(const std::wstring& endpoint_id, std::uint8_t level_percent = 100);
    bool busy() const noexcept { return busy_.load(std::memory_order_acquire); }
    AudioPreviewResult result() const noexcept { return result_.load(std::memory_order_acquire); }
    void wait();
private:
    std::thread thread_;
    std::atomic<bool> busy_{false};
    std::atomic<AudioPreviewResult> result_{AudioPreviewResult::NotRun};
};
const wchar_t* audio_preview_diagnostic(AudioPreviewResult result) noexcept;
} // namespace axv
