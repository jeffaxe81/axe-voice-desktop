#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <ksmedia.h>
#include <objbase.h>
#include <memory>
#include "audio_preview.h"
#include "audio_preview_tone.h"
#include "audio_preview_levels.h"

namespace axv {
namespace {
template<class T> struct ComPtr {
    T* ptr = nullptr;
    ComPtr() = default;
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
    ~ComPtr() { if (ptr) ptr->Release(); }
    T** out() { return &ptr; }
    T* operator->() const { return ptr; }
};
struct ComApartment {
    const HRESULT status = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ~ComApartment() { if (SUCCEEDED(status)) CoUninitialize(); }
};
bool detect_tone_encoding(const WAVEFORMATEX* fmt, ToneSampleEncoding& encoding) {
    if (!fmt || !fmt->nChannels || fmt->nChannels > 8 ||
        fmt->nSamplesPerSec < 8000 || fmt->nSamplesPerSec > 192000) return false;
    const bool is_extended = fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        fmt->cbSize >= sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
    const auto* ext = is_extended ? reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(fmt) : nullptr;
    const bool is_float = fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (ext && IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT));
    const bool is_pcm = fmt->wFormatTag == WAVE_FORMAT_PCM ||
        (ext && IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_PCM));
    if (is_float && fmt->wBitsPerSample == 32 &&
        fmt->nBlockAlign == fmt->nChannels * sizeof(float)) {
        encoding = ToneSampleEncoding::Float32;
        return true;
    }
    if (is_pcm && fmt->wBitsPerSample == 16 &&
        fmt->nBlockAlign == fmt->nChannels * sizeof(std::int16_t)) {
        encoding = ToneSampleEncoding::Signed16;
        return true;
    }
    return false;
}
} // namespace

AudioPreviewResult play_preview_tone_on_endpoint(
    const std::wstring& endpoint_id, std::uint8_t level_percent) {
    if (level_percent > 100) return AudioPreviewResult::PlaybackFailed;
    if (endpoint_id.empty()) return AudioPreviewResult::NoEndpoint;
    ComApartment com;
    if (FAILED(com.status)) return AudioPreviewResult::WindowsAudioUnavailable;
    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
        CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(enumerator.out()))) || !enumerator.ptr)
        return AudioPreviewResult::WindowsAudioUnavailable;
    ComPtr<IMMDevice> device;
    if (FAILED(enumerator->GetDevice(endpoint_id.c_str(), device.out())) || !device.ptr)
        return AudioPreviewResult::NoEndpoint;
    ComPtr<IAudioClient> client;
    if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER,
        nullptr, reinterpret_cast<void**>(client.out()))) || !client.ptr)
        return AudioPreviewResult::WindowsAudioUnavailable;
    WAVEFORMATEX* format_ptr = nullptr;
    if (FAILED(client->GetMixFormat(&format_ptr)) || !format_ptr)
        return AudioPreviewResult::WindowsAudioUnavailable;
    std::unique_ptr<WAVEFORMATEX, decltype(&CoTaskMemFree)> format(format_ptr, &CoTaskMemFree);
    ToneSampleEncoding encoding = ToneSampleEncoding::Float32;
    if (!detect_tone_encoding(format.get(), encoding))
        return AudioPreviewResult::UnsupportedFormat;
    constexpr REFERENCE_TIME kBufferDuration100ns = 4000000; // 400 ms
    if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                                  kBufferDuration100ns, 0, format.get(), nullptr)))
        return AudioPreviewResult::PlaybackFailed;
    UINT32 frames = 0;
    if (FAILED(client->GetBufferSize(&frames)) || !frames ||
        frames > format->nSamplesPerSec * 2u)
        return AudioPreviewResult::PlaybackFailed;
    ComPtr<IAudioRenderClient> renderer;
    if (FAILED(client->GetService(__uuidof(IAudioRenderClient),
        reinterpret_cast<void**>(renderer.out()))) || !renderer.ptr)
        return AudioPreviewResult::PlaybackFailed;
    BYTE* samples = nullptr;
    if (FAILED(renderer->GetBuffer(frames, &samples)) || !samples)
        return AudioPreviewResult::PlaybackFailed;
    const bool generated = fill_preview_tone(samples, frames,
        format->nChannels, format->nSamplesPerSec, encoding);
    const bool scaled = generated && scale_preview_buffer(
        samples, frames, format->nChannels, encoding, level_percent);
    const HRESULT released = renderer->ReleaseBuffer(
        frames, scaled ? 0 : AUDCLNT_BUFFERFLAGS_SILENT);
    if (!scaled || FAILED(released)) return AudioPreviewResult::PlaybackFailed;
    if (FAILED(client->Start())) return AudioPreviewResult::PlaybackFailed;
    // Intentional, bounded local preview. This runs only on the worker thread.
    Sleep(310);
    if (FAILED(client->Stop())) return AudioPreviewResult::PlaybackFailed;
    return AudioPreviewResult::Completed;
}

bool AudioPreviewWorker::start(const std::wstring& endpoint_id,
                               std::uint8_t level_percent) {
    if (endpoint_id.empty() || level_percent > 100 || busy()) return false;
    wait(); // Previous completed worker, not the active one.
    busy_.store(true, std::memory_order_release);
    result_.store(AudioPreviewResult::NotRun, std::memory_order_release);
    try {
        thread_ = std::thread([this, endpoint_id, level_percent] {
            result_.store(play_preview_tone_on_endpoint(endpoint_id, level_percent),
                          std::memory_order_release);
            busy_.store(false, std::memory_order_release);
        });
    } catch (...) {
        busy_.store(false, std::memory_order_release);
        result_.store(AudioPreviewResult::PlaybackFailed, std::memory_order_release);
        return false;
    }
    return true;
}
void AudioPreviewWorker::wait() {
    if (thread_.joinable()) thread_.join();
}
const wchar_t* audio_preview_diagnostic(AudioPreviewResult result) noexcept {
    switch (result) {
    case AudioPreviewResult::Completed:
        return L"Som local enviado ao dispositivo. Voz SIP continua indisponivel.";
    case AudioPreviewResult::NoEndpoint:
        return L"Saida selecionada nao esta disponivel.";
    case AudioPreviewResult::WindowsAudioUnavailable:
        return L"Windows nao permitiu abrir o dispositivo de audio.";
    case AudioPreviewResult::UnsupportedFormat:
        return L"Formato do dispositivo nao suportado pelo teste (PCM16/float32).";
    case AudioPreviewResult::PlaybackFailed:
        return L"Falha ao reproduzir o som local. Verifique volume/headset.";
    case AudioPreviewResult::NotRun:
        return L"Nenhum teste de audio executado.";
    }
    return L"Falha desconhecida do teste de audio.";
}
} // namespace axv
