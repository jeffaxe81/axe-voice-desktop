#pragma once
#include <atomic>
#include <string>
#include <thread>

namespace axv {

enum class MicrophoneTestState {
    Idle, Capturing, SignalDetected, Quiet, MissingDevice, UnsupportedFormat, Failed, Cancelled
};

// Captures at most ~850 ms of Windows microphone data for a transient peak
// classification. No raw audio leaves WASAPI; no file/network/telephony access.
MicrophoneTestState test_windows_microphone(
    const std::wstring& endpoint_id, const std::atomic<bool>* cancel = nullptr);

class MicrophoneTestWorker {
public:
    MicrophoneTestWorker() = default;
    MicrophoneTestWorker(const MicrophoneTestWorker&) = delete;
    MicrophoneTestWorker& operator=(const MicrophoneTestWorker&) = delete;
    ~MicrophoneTestWorker();
    bool start(const std::wstring& endpoint_id);
    void request_stop() { cancel_.store(true); }
    MicrophoneTestState state() const { return state_.load(); }
private:
    std::thread worker_;
    std::atomic<bool> cancel_{false};
    std::atomic<MicrophoneTestState> state_{MicrophoneTestState::Idle};
};

const wchar_t* microphone_test_feedback(MicrophoneTestState state);

} // namespace axv
