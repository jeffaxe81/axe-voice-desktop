#include "audio_endpoint_watcher.h"
#include <new>
#include <objbase.h>

namespace axv {
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::QueryInterface(
    REFIID iid, void** result) {
    if (!result) return E_POINTER;
    *result = nullptr;
    if (iid == __uuidof(IUnknown) || iid == __uuidof(IMMNotificationClient)) {
        *result = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE AudioEndpointNotificationSink::AddRef() {
    return refs_.fetch_add(1, std::memory_order_relaxed) + 1;
}
ULONG STDMETHODCALLTYPE AudioEndpointNotificationSink::Release() {
    const ULONG count = refs_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (!count) delete this;
    return count;
}
void AudioEndpointNotificationSink::wake_gui() noexcept {
    if (recipient_ && message_) PostMessageW(recipient_, message_, 0, 0);
}
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDeviceStateChanged(
    LPCWSTR, DWORD) { wake_gui(); return S_OK; }
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDeviceAdded(
    LPCWSTR) { wake_gui(); return S_OK; }
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDeviceRemoved(
    LPCWSTR) { wake_gui(); return S_OK; }
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDefaultDeviceChanged(
    EDataFlow flow, ERole role, LPCWSTR) {
    // Voice follows eCommunications; local ringtone preview follows eMultimedia.
    if ((flow == eCapture || flow == eRender) &&
        (role == eCommunications || role == eMultimedia)) wake_gui();
    return S_OK;
}
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnPropertyValueChanged(
    LPCWSTR, const PROPERTYKEY) { wake_gui(); return S_OK; }

bool AudioEndpointWatcher::start(HWND recipient, UINT message) {
    if (com_ready_ || enumerator_ || callback_ || !IsWindow(recipient) ||
        message < WM_APP) return false;
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(init)) return false; // WM_DEVICECHANGE/manual refresh remain available.
    com_ready_ = true;
    const HRESULT created = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&enumerator_));
    if (FAILED(created) || !enumerator_) {
        stop();
        return false;
    }
    callback_ = new (std::nothrow) AudioEndpointNotificationSink(recipient, message);
    if (!callback_) {
        stop();
        return false;
    }
    const HRESULT registered = enumerator_->RegisterEndpointNotificationCallback(callback_);
    if (FAILED(registered)) {
        stop();
        return false;
    }
    registered_ = true;
    return true;
}
void AudioEndpointWatcher::stop() noexcept {
    // Unregister before HWND teardown. In-flight callbacks only post hints,
    // never touch the owner or its SIP engine.
    if (registered_ && enumerator_ && callback_)
        enumerator_->UnregisterEndpointNotificationCallback(callback_);
    registered_ = false;
    if (callback_) {
        callback_->Release();
        callback_ = nullptr;
    }
    if (enumerator_) {
        enumerator_->Release();
        enumerator_ = nullptr;
    }
    if (com_ready_) {
        CoUninitialize();
        com_ready_ = false;
    }
}
} // namespace axv
