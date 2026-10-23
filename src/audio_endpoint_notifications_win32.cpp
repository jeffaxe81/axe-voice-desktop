#include "audio_endpoint_notifications_win32.h"
#include <objbase.h>

namespace axv {
AudioEndpointNotificationSink::AudioEndpointNotificationSink(HWND target, UINT message)
    : target_(target), message_(message) {}

void AudioEndpointNotificationSink::detach() {
    std::lock_guard<std::mutex> lock(target_mutex_);
    target_ = nullptr;
}

void AudioEndpointNotificationSink::signal() {
    std::lock_guard<std::mutex> lock(target_mutex_);
    if (target_ && message_) (void)PostMessageW(target_, message_, 0, 0);
}

HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::QueryInterface(
        REFIID iid, void** object) {
    if (!object) return E_POINTER;
    *object = nullptr;
    if (iid == __uuidof(IUnknown) || iid == __uuidof(IMMNotificationClient)) {
        *object = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE AudioEndpointNotificationSink::AddRef() {
    return ++refs_;
}
ULONG STDMETHODCALLTYPE AudioEndpointNotificationSink::Release() {
    const ULONG n = --refs_;
    if (!n) delete this;
    return n;
}

HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDeviceStateChanged(LPCWSTR, DWORD) {
    signal(); return S_OK;
}
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDeviceAdded(LPCWSTR) {
    signal(); return S_OK;
}
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDeviceRemoved(LPCWSTR) {
    signal(); return S_OK;
}
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnDefaultDeviceChanged(
        EDataFlow, ERole, LPCWSTR) {
    signal(); return S_OK;
}
HRESULT STDMETHODCALLTYPE AudioEndpointNotificationSink::OnPropertyValueChanged(
        LPCWSTR, const PROPERTYKEY) {
    signal(); return S_OK;
}

bool AudioEndpointNotifier::start(HWND target, UINT message) {
    if (enumerator_ || sink_ || !target || message < WM_APP) return false;
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE) return false;
    co_initialized_ = SUCCEEDED(init);
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&enumerator_));
    if (FAILED(hr) || !enumerator_) {
        stop();
        return false;
    }
    sink_ = new AudioEndpointNotificationSink(target, message);
    hr = enumerator_->RegisterEndpointNotificationCallback(sink_);
    if (FAILED(hr)) {
        stop();
        return false;
    }
    return true;
}

void AudioEndpointNotifier::stop() {
    if (sink_) sink_->detach(); // Fence callback posting before HWND teardown.
    if (enumerator_ && sink_)
        (void)enumerator_->UnregisterEndpointNotificationCallback(sink_);
    if (sink_) { sink_->Release(); sink_ = nullptr; }
    if (enumerator_) { enumerator_->Release(); enumerator_ = nullptr; }
    if (co_initialized_) { CoUninitialize(); co_initialized_ = false; }
}
} // namespace axv
