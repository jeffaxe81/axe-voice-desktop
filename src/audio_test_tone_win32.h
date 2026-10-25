#pragma once
#include <atomic>
#include <string>
#include <thread>

namespace axv {

enum class TonePreviewState {
    Idle, Playing, Played, MissingDevice, UnsupportedFormat, Failed, Cancelled
};

// Render-only250ms WASAPI primitive. No microphone capture, SIP or RTP.
// Empty IDs require explicit multimedia-default mode; cancellation is cooperative.
TonePreviewState play_windows_tone_on_endpoint(const std::wstring& endpoint_id,
    const std::atomic<bool>* cancel=nullptr,bool multimedia_default=false);

class TonePreviewPlayer {
public:
    TonePreviewPlayer() = default;
    TonePreviewPlayer(const TonePreviewPlayer&) = delete;
    TonePreviewPlayer& operator=(const TonePreviewPlayer&) = delete;
    ~TonePreviewPlayer();

    // Empty IDs require explicit multimedia-default mode; concurrent preview/thread errors reject.
    bool start(const std::wstring& endpoint_id,bool multimedia_default=false);
    void request_stop(){cancel_.store(true);}
    TonePreviewState state() const { return state_.load(); }

private:
    std::thread worker_;
    std::atomic<bool> cancel_{false};
    std::atomic<TonePreviewState> state_{TonePreviewState::Idle};
};

const wchar_t* tone_preview_feedback(TonePreviewState state);
} // namespace axv
