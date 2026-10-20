#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include "audio_devices.h"

namespace axv {
namespace {

bool count_active(IMMDeviceEnumerator* enumerator, EDataFlow flow, unsigned int& count) {
    IMMDeviceCollection* collection = nullptr;
    const HRESULT hr = enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection);
    if (FAILED(hr) || !collection) return false;
    UINT amount = 0;
    const HRESULT count_result = collection->GetCount(&amount);
    collection->Release();
    if (FAILED(count_result)) return false;
    count = amount;
    return true;
}

bool has_default(IMMDeviceEnumerator* enumerator, EDataFlow flow) {
    IMMDevice* device = nullptr;
    const HRESULT hr = enumerator->GetDefaultAudioEndpoint(flow, eCommunications, &device);
    if (device) device->Release();
    return SUCCEEDED(hr);
}

} // namespace

AudioInventory inspect_windows_audio_devices() {
    AudioInventory result;
    const HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // CoInitializeEx must be balanced for S_OK and S_FALSE alike.
    if (FAILED(initialization)) {
        result.error = AudioInventoryError::ComInitializationFailed;
        return result;
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    const HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr) || !enumerator) {
        result.error = AudioInventoryError::EnumeratorUnavailable;
    } else {
        if (!count_active(enumerator, eCapture, result.input_count) ||
            !count_active(enumerator, eRender, result.output_count)) {
            result.error = AudioInventoryError::EndpointQueryFailed;
        } else {
            result.has_default_communications_input = has_default(enumerator, eCapture);
            result.has_default_communications_output = has_default(enumerator, eRender);
        }
        enumerator->Release();
    }
    CoUninitialize();
    return result;
}

} // namespace axv
