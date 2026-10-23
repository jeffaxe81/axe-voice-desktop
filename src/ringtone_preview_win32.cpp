#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include "ringtone_preview.h"

namespace axv {
namespace {
bool is_standard_subformat(const GUID& sub, unsigned tag) {
    return sub.Data1 == tag && sub.Data2 == 0 &&
           sub.Data3 == 0x0010 &&
           sub.Data4[0] == 0x80 && sub.Data4[1] == 0 &&
           sub.Data4[2] == 0 && sub.Data4[3] == 0xaa &&
           sub.Data4[4] == 0 && sub.Data4[5] == 0x38 &&
           sub.Data4[6] == 0x9b && sub.Data4[7] == 0x71;
}
bool supported_mix(const WAVEFORMATEX* mix, RingSampleFormat& result) {
    if (!mix || !mix->nChannels || mix->nChannels > 8 ||
        mix->nSamplesPerSec < 8000 || mix->nSamplesPerSec > 192000)
        return false;
    bool floating = mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
    bool pcm = mix->wFormatTag == WAVE_FORMAT_PCM;
    if (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        mix->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
        const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(mix);
        floating = is_standard_subformat(ext->SubFormat, WAVE_FORMAT_IEEE_FLOAT);
        pcm = is_standard_subformat(ext->SubFormat, WAVE_FORMAT_PCM);
    }
    if (floating && mix->wBitsPerSample == 32 &&
        mix->nBlockAlign == mix->nChannels * sizeof(float)) {
        result = RingSampleFormat::Float32;
        return true;
    }
    if (pcm && mix->wBitsPerSample == 16 &&
        mix->nBlockAlign == mix->nChannels * sizeof(std::int16_t)) {
        result = RingSampleFormat::Pcm16;
        return true;
    }
    return false;
}
} // namespace

RingPreviewResult play_selected_ringtone_preview(
    const std::wstring& endpoint_id, const std::atomic<bool>* cancellation) {
    if (ring_preview_cancelled(cancellation)) return RingPreviewResult::Cancelled;
    if (endpoint_id.empty() || endpoint_id.size() > 4096)
        return RingPreviewResult::DeviceUnavailable;

    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return RingPreviewResult::AudioServiceUnavailable;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* client = nullptr;
    IAudioRenderClient* renderer = nullptr;
    WAVEFORMATEX* mix = nullptr;
    auto result = RingPreviewResult::AudioServiceUnavailable;
    do {
        if (ring_preview_cancelled(cancellation)) {
            result = RingPreviewResult::Cancelled; break;
        }
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(&enumerator))) || !enumerator) break;
        if (FAILED(enumerator->GetDevice(endpoint_id.c_str(), &device)) || !device) {
            result = RingPreviewResult::DeviceUnavailable; break;
        }
        DWORD state = 0;
        if (FAILED(device->GetState(&state)) || !(state & DEVICE_STATE_ACTIVE)) {
            result = RingPreviewResult::DeviceUnavailable; break;
        }
        if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                nullptr, reinterpret_cast<void**>(&client))) || !client) break;
        if (FAILED(client->GetMixFormat(&mix)) || !mix) break;
        RingSampleFormat encoding = RingSampleFormat::Float32;
        if (!supported_mix(mix, encoding)) {
            result = RingPreviewResult::UnsupportedFormat; break;
        }
        // Shared mode; do not take exclusive control of the speaker.
        constexpr REFERENCE_TIME buffer_duration = 4000000; // 400ms
        if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                buffer_duration, 0, mix, nullptr))) break;
        UINT32 frames = 0;
        if (FAILED(client->GetBufferSize(&frames)) || frames == 0 ||
            frames > static_cast<UINT32>(mix->nSamplesPerSec) * 2) break;
        if (FAILED(client->GetService(__uuidof(IAudioRenderClient),
                reinterpret_cast<void**>(&renderer))) || !renderer) break;
        BYTE* buffer = nullptr;
        if (FAILED(renderer->GetBuffer(frames, &buffer)) || !buffer) break;
        const bool filled = render_ring_samples(buffer, frames,
            mix->nChannels, mix->nSamplesPerSec, encoding);
        if (FAILED(renderer->ReleaseBuffer(frames,
                    filled ? 0 : AUDCLNT_BUFFERFLAGS_SILENT)) || !filled) break;
        if (ring_preview_cancelled(cancellation)) {
            result = RingPreviewResult::Cancelled; break;
        }
        if (FAILED(client->Start())) break;
        result = RingPreviewResult::Played;
        // Short cooperative waits: UI can cancel without a 380ms blocked join.
        for (int elapsed = 0; elapsed < 380; elapsed += 10) {
            if (ring_preview_cancelled(cancellation)) {
                result = RingPreviewResult::Cancelled;
                break;
            }
            Sleep(10);
        }
        const HRESULT stopped = client->Stop();
        if (ring_preview_cancelled(cancellation)) result = RingPreviewResult::Cancelled;
        else if (FAILED(stopped)) result = RingPreviewResult::AudioServiceUnavailable;
    } while (false);
    if (renderer) renderer->Release();
    if (mix) CoTaskMemFree(mix);
    if (client) client->Release();
    if (device) device->Release();
    if (enumerator) enumerator->Release();
    CoUninitialize();
    return ring_preview_cancelled(cancellation) ? RingPreviewResult::Cancelled : result;
}
} // namespace axv
