#pragma once
#ifndef _WIN32
#error Windows audio endpoint notifications require Windows
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <atomic>

namespace axv {

// Only posts a wake-up to the GUI thread; no COM reentry, device ID copies,
// SIP commands or audio device enumeration inside a Windows callback.
class AudioEndpointNotificationSink final : public IMMNotificationClient {
public:
    AudioEndpointNotificationSink(HWND recipient, UINT message) noexcept
        : recipient_(recipient), message_(message) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR id, DWORD state) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR id) override;
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR id) override;
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                     LPCWSTR id) override;
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR id, const PROPERTYKEY key) override;

private:
    ~AudioEndpointNotificationSink() override = default;
    void wake_gui() noexcept;
    std::atomic<ULONG> refs_{1};
    const HWND recipient_;
    const UINT message_;
};

// Register/unregister on the same Win32 UI thread. The watcher owns its
// COM objects; call stop() during WM_DESTROY before the HWND is destroyed.
class AudioEndpointWatcher final {
public:
    AudioEndpointWatcher() = default;
    ~AudioEndpointWatcher() { stop(); }
    AudioEndpointWatcher(const AudioEndpointWatcher&) = delete;
    AudioEndpointWatcher& operator=(const AudioEndpointWatcher&) = delete;

    bool start(HWND recipient, UINT message);
    void stop() noexcept;
    bool active() const noexcept { return registered_; }
private:
    IMMDeviceEnumerator* enumerator_ = nullptr;
    AudioEndpointNotificationSink* callback_ = nullptr;
    bool com_ready_ = false;
    bool registered_ = false;
};
} // namespace axv
