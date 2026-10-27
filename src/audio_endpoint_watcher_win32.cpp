#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>

#include "audio_endpoint_watcher_win32.h"
#include "audio_device_change_signal.h"

#include <atomic>
#include <new>
#include <utility>

namespace axv {
namespace {

class EndpointNotificationClient final : public IMMNotificationClient {
public:
    explicit EndpointNotificationClient(std::shared_ptr<AudioDeviceChangeSignal> signal)
        : signal_(std::move(signal)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, __uuidof(IMMNotificationClient))) {
            *object = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --references_;
        if (!remaining) delete this;
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override {
        signal_->notify();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override {
        signal_->notify();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override {
        signal_->notify();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                      LPCWSTR) override {
        if ((flow == eCapture || flow == eRender) &&
            (role == eMultimedia || role == eCommunications))
            signal_->notify();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override {
        signal_->notify();
        return S_OK;
    }

private:
    std::atomic<ULONG> references_{1};
    const std::shared_ptr<AudioDeviceChangeSignal> signal_;
};

} // namespace

struct AudioEndpointWatcher::Impl {
    std::shared_ptr<AudioDeviceChangeSignal> signal;
    IMMDeviceEnumerator* enumerator = nullptr;
    EndpointNotificationClient* sink = nullptr;
    bool com_initialized = false;
    bool registered = false;

    void stop() {
        // Closing first also makes notifications from retained/in-flight COM
        // callbacks harmless. Their sink owns the old signal across restarts.
        if (signal) signal->close();
        if (registered) {
            enumerator->UnregisterEndpointNotificationCallback(sink);
            registered = false;
        }
        // Registration does not AddRef the client: keep our reference until
        // after unregister, and never unregister/release COM from a callback.
        if (enumerator) { enumerator->Release(); enumerator = nullptr; }
        if (sink) { sink->Release(); sink = nullptr; }
        signal.reset();
        if (com_initialized) {
            CoUninitialize();
            com_initialized = false;
        }
    }
};

AudioEndpointWatcher::AudioEndpointWatcher() : impl_(new Impl) {}
AudioEndpointWatcher::~AudioEndpointWatcher() { stop(); }

bool AudioEndpointWatcher::start() {
    if (impl_->registered) return true;
    impl_->stop();
    try {
        impl_->signal = std::make_shared<AudioDeviceChangeSignal>();
        impl_->sink = new EndpointNotificationClient(impl_->signal);
    } catch (const std::bad_alloc&) {
        impl_->stop();
        return false;
    }
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        impl_->stop();
        return false;
    }
    // S_FALSE also increments COM's initialization count.
    impl_->com_initialized = true;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                               CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                               reinterpret_cast<void**>(&impl_->enumerator))) ||
        !impl_->enumerator) {
        impl_->stop();
        return false;
    }
    if (FAILED(impl_->enumerator->RegisterEndpointNotificationCallback(impl_->sink))) {
        impl_->stop();
        return false;
    }
    impl_->registered = true;
    return true;
}

void AudioEndpointWatcher::stop() { impl_->stop(); }
bool AudioEndpointWatcher::take_changed() {
    return impl_->registered && impl_->signal && impl_->signal->take();
}
bool AudioEndpointWatcher::active() const { return impl_->registered; }

} // namespace axv
