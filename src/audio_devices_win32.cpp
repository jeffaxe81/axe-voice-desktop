#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <propvarutil.h>
#include <functiondiscoverykeys_devpkey.h>
#include "audio_devices.h"
#include <string>
#include <utility>

namespace axv {
namespace {
std::wstring default_endpoint_id(IMMDeviceEnumerator* enumerator, EDataFlow flow, ERole role) {
    IMMDevice* device = nullptr;
    if (FAILED(enumerator->GetDefaultAudioEndpoint(flow, role, &device)) || !device)
        return {};
    LPWSTR id = nullptr;
    const HRESULT hr = device->GetId(&id);
    const std::wstring result = SUCCEEDED(hr) && id ? std::wstring(id) : std::wstring();
    if (id) CoTaskMemFree(id);
    device->Release();
    return result;
}

std::wstring friendly_name(IMMDevice* device) {
    IPropertyStore* props = nullptr;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &props)) || !props)
        return L"Dispositivo sem nome";
    PROPVARIANT prop;
    PropVariantInit(&prop);
    std::wstring result = L"Dispositivo sem nome";
    if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &prop)) &&
        prop.vt == VT_LPWSTR && prop.pwszVal && prop.pwszVal[0]) {
        result = std::wstring(prop.pwszVal).substr(0, 96);
    }
    PropVariantClear(&prop);
    props->Release();
    return result;
}

bool collect(IMMDeviceEnumerator* enumerator, EDataFlow flow,
             std::vector<AudioEndpoint>& devices, unsigned int& count, bool& has_default) {
    IMMDeviceCollection* collection = nullptr;
    if (FAILED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection)) ||
        !collection) return false;
    UINT total = 0;
    if (FAILED(collection->GetCount(&total)) || total > 128) {
        collection->Release();
        return false;
    }
    const std::wstring default_id = default_endpoint_id(enumerator, flow, eCommunications);
    const std::wstring multimedia_id = flow == eRender
        ? default_endpoint_id(enumerator, flow, eMultimedia) : std::wstring{};
    devices.clear();
    devices.reserve(total);
    bool ok = true;
    for (UINT i = 0; i < total; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(collection->Item(i, &device)) || !device) { ok = false; break; }
        LPWSTR id = nullptr;
        if (FAILED(device->GetId(&id)) || !id) {
            if (id) CoTaskMemFree(id);
            device->Release();
            ok = false;
            break;
        }
        AudioEndpoint item;
        item.id = id;
        CoTaskMemFree(id);
        item.name = friendly_name(device);
        item.default_communications = !default_id.empty() && item.id == default_id;
        item.default_multimedia = !multimedia_id.empty() && item.id == multimedia_id;
        devices.push_back(std::move(item));
        device->Release();
    }
    collection->Release();
    if (!ok) { devices.clear(); return false; }
    count = static_cast<unsigned int>(devices.size());
    has_default = !default_id.empty();
    return true;
}
} // namespace

AudioInventory inspect_windows_audio_devices() {
    AudioInventory result;
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(init)) {
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
        if (!collect(enumerator, eCapture, result.input_devices,
                     result.input_count, result.has_default_communications_input) ||
            !collect(enumerator, eRender, result.output_devices,
                     result.output_count, result.has_default_communications_output)) {
            result.error = AudioInventoryError::EndpointQueryFailed;
            result.input_devices.clear(); result.output_devices.clear();
            result.input_count = result.output_count = 0;
            result.has_default_communications_input = false;
            result.has_default_communications_output = false;
        }
        enumerator->Release();
    }
    CoUninitialize();
    return result;
}
} // namespace axv
