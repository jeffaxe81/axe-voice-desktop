#pragma once
#include <atomic>
#include <string>
#include <thread>

namespace axv {

enum class TonePreviewState {
    Idle, Playing, Played, MissingDevice, UnsupportedFormat, Failed
};

// A short, user-initiated local Windows WASAPI test. No microphone capture,
// SIP account, RTP, background ringtone, or Baresip engine is involved.
TonePreviewState play_windows_tone_on_endpoint(const std::wstring& endpoint_id);

class TonePreviewPlayer {
public:
    TonePreviewPlayer() = default;
    TonePreviewPlayer(const TonePreviewPlayer&) = delete;
    TonePreviewPlayer& operator=(const TonePreviewPlayer&) = delete;
    ~TonePreviewPlayer();

    // Returns false for empty endpoints, a concurrent preview or thread error.
    bool start(const std::wstring& endpoint_id);
    TonePreviewState state() const { return state_.load(); }

private:
    std::thread worker_;
    std::atomic<TonePreviewState> state_{TonePreviewState::Idle};
};

const wchar_t* tone_preview_feedback(TonePreviewState state);
} // namespace axv
