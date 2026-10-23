#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <atomic>
#include <mutex>

namespace axv {

// COM callback transport: only posts an application-defined message. Device
// identifiers, property values, SIP credentials and audio streams never cross
// the callback/UI boundary.
class AudioEndpointNotificationSink final : public IMMNotificationClient {
public:
    AudioEndpointNotificationSink(HWND target, UINT message);
    void detach();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override;
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override;
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR) override;
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override;

private:
    void signal();
    ~AudioEndpointNotificationSink() = default;
    std::atomic<ULONG> refs_{1};
    std::mutex target_mutex_;
    HWND target_ = nullptr;
    UINT message_ = 0;
};

// Called only from the Win32 UI thread, including stop() during WM_DESTROY.
// Failure to register is not fatal: 3-second polling remains enabled.
class AudioEndpointNotifier {
public:
    AudioEndpointNotifier() = default;
    AudioEndpointNotifier(const AudioEndpointNotifier&) = delete;
    AudioEndpointNotifier& operator=(const AudioEndpointNotifier&) = delete;
    ~AudioEndpointNotifier() { stop(); }

    bool start(HWND target, UINT message);
    void stop();
private:
    IMMDeviceEnumerator* enumerator_ = nullptr;
    AudioEndpointNotificationSink* sink_ = nullptr;
    bool co_initialized_ = false;
};
} // namespace axv
