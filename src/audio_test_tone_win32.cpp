#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "audio_test_tone.h"
#include "audio_test_tone_win32.h"
#include <algorithm>
#include <cstdint>
#include <utility>

namespace axv {
namespace {
template<class T> struct ScopedCom {
    T* ptr = nullptr;
    ~ScopedCom() { if (ptr) ptr->Release(); }
    T** put() { return &ptr; }
    T* operator->() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
};

struct ScopedFormat {
    WAVEFORMATEX* ptr = nullptr;
    ~ScopedFormat() { if (ptr) CoTaskMemFree(ptr); }
};

TonePreviewState render_preview(
    const std::wstring& endpoint_id, const std::atomic<bool>* cancel_requested) {
    const auto cancelled = [cancel_requested] {
        return cancel_requested && cancel_requested->load();
    };
    if (cancelled()) return TonePreviewState::Cancelled;
    if (endpoint_id.empty() || endpoint_id.size() > 4096)
        return TonePreviewState::MissingDevice;

    // This is a short-lived worker, not the UI thread. Baresip has no role.
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(init)) return TonePreviewState::Failed;
    const auto render = [&]() -> TonePreviewState {
        if (cancelled()) return TonePreviewState::Cancelled;
        ScopedCom<IMMDeviceEnumerator> enumerator;
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(enumerator.put()))) || !enumerator)
            return TonePreviewState::Failed;

        ScopedCom<IMMDevice> device;
        if (FAILED(enumerator->GetDevice(endpoint_id.c_str(), device.put())) || !device)
            return TonePreviewState::MissingDevice;
        DWORD state = 0;
        if (FAILED(device->GetState(&state)) || state != DEVICE_STATE_ACTIVE)
            return TonePreviewState::MissingDevice;

        ScopedCom<IAudioClient> audio;
        if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER,
                nullptr, reinterpret_cast<void**>(audio.put()))) || !audio)
            return TonePreviewState::Failed;
        ScopedFormat mix;
        if (FAILED(audio->GetMixFormat(&mix.ptr)) || !mix.ptr)
            return TonePreviewState::Failed;

        const auto* wfx = mix.ptr;
        ToneSampleType type;
        WORD tag = wfx->wFormatTag;
        if (tag == WAVE_FORMAT_EXTENSIBLE) {
            if (wfx->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
                return TonePreviewState::UnsupportedFormat;
            const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);
            if (IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
                tag = WAVE_FORMAT_IEEE_FLOAT;
            else if (IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
                tag = WAVE_FORMAT_PCM;
            else
                return TonePreviewState::UnsupportedFormat;
        }
        if (tag == WAVE_FORMAT_IEEE_FLOAT && wfx->wBitsPerSample == 32)
            type = ToneSampleType::Float32;
        else if (tag == WAVE_FORMAT_PCM && wfx->wBitsPerSample == 16)
            type = ToneSampleType::Pcm16;
        else if (tag == WAVE_FORMAT_PCM && wfx->wBitsPerSample == 32)
            type = ToneSampleType::Pcm32;
        else
            return TonePreviewState::UnsupportedFormat;

        const ToneFormat spec{wfx->nSamplesPerSec, wfx->nChannels, type};
        if (!tone_format_valid(spec) ||
            wfx->nBlockAlign != spec.channels * tone_sample_bytes(type) ||
            !tone_total_frames(spec)) return TonePreviewState::UnsupportedFormat;

        // Shared mode respects Windows mixer/device routing; no exclusive
        // hardware claim or change to system default endpoint.
        if (FAILED(audio->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                  2000000, 0, wfx, nullptr))) return TonePreviewState::Failed;
        UINT32 capacity = 0;
        if (FAILED(audio->GetBufferSize(&capacity)) || !capacity)
            return TonePreviewState::Failed;
        ScopedCom<IAudioRenderClient> sink;
        if (FAILED(audio->GetService(__uuidof(IAudioRenderClient),
                reinterpret_cast<void**>(sink.put()))) || !sink)
            return TonePreviewState::Failed;

        if (cancelled()) return TonePreviewState::Cancelled;
        if (FAILED(audio->Start())) return TonePreviewState::Failed;
        TonePreviewState result = TonePreviewState::Failed;
        UINT32 submitted = 0;
        const UINT32 total = tone_total_frames(spec);
        // Bound the loop: a stopped/disconnected endpoint must never hang UI
        // shutdown. The generation is private to this one preview request.
        for (int attempt = 0; attempt < 130 && submitted < total; ++attempt) {
            if (cancelled()) { result = TonePreviewState::Cancelled; break; }
            UINT32 padding = 0;
            if (FAILED(audio->GetCurrentPadding(&padding)) || padding > capacity)
                break;
            const UINT32 chunk = std::min(total - submitted, capacity - padding);
            if (chunk) {
                BYTE* samples = nullptr;
                if (FAILED(sink->GetBuffer(chunk, &samples)) || !samples) break;
                const bool filled = write_tone_packet(
                    samples,
                    static_cast<std::size_t>(chunk) * wfx->nBlockAlign,
                    spec, submitted, chunk);
                const HRESULT released = sink->ReleaseBuffer(
                    chunk, filled ? 0 : AUDCLNT_BUFFERFLAGS_SILENT);
                if (!filled || FAILED(released)) break;
                submitted += chunk;
            } else {
                Sleep(10);
            }
            if (submitted == total) {
                // Give Windows time to drain the remaining shared-mode
                // buffer before stopping playback.
                for (int step = 0; step < 22 && !cancelled(); ++step)
                    Sleep(10);
                result = cancelled() ? TonePreviewState::Cancelled
                                     : TonePreviewState::Played;
                break;
            }
        }
        audio->Stop();
        return result;
    };
    TonePreviewState result = TonePreviewState::Failed;
    try { result = render(); } catch (...) { result = TonePreviewState::Failed; }
    CoUninitialize();
    return result;
}
} // namespace

TonePreviewState play_windows_tone_on_endpoint(
    const std::wstring& endpoint_id, const std::atomic<bool>* cancel_requested) {
    return render_preview(endpoint_id, cancel_requested);
}

TonePreviewPlayer::~TonePreviewPlayer() {
    cancel_and_join();
}
bool TonePreviewPlayer::start(const std::wstring& endpoint_id) {
    if (endpoint_id.empty() || state_.load() == TonePreviewState::Playing)
        return false;
    if (worker_.joinable()) worker_.join();
    cancel_requested_.store(false);
    state_.store(TonePreviewState::Playing);
    try {
        worker_ = std::thread([this, id = endpoint_id] {
            TonePreviewState result = TonePreviewState::Failed;
            try { result = play_windows_tone_on_endpoint(id, &cancel_requested_); }
            catch (...) { result = TonePreviewState::Failed; }
            state_.store(cancel_requested_.load() ? TonePreviewState::Cancelled : result);
        });
    } catch (...) {
        state_.store(TonePreviewState::Failed);
        return false;
    }
    return true;
}
bool TonePreviewPlayer::cancel() {
    if (state_.load() != TonePreviewState::Playing) return false;
    cancel_requested_.store(true);
    return true;
}
void TonePreviewPlayer::cancel_and_join() {
    cancel_requested_.store(true);
    if (worker_.joinable()) worker_.join();
}
const wchar_t* tone_preview_feedback(TonePreviewState state) {
    switch (state) {
    case TonePreviewState::Idle:
        return L"Teste de toque disponivel apos selecionar uma saida.";
    case TonePreviewState::Playing:
        return L"Reproduzindo toque local no dispositivo escolhido...";
    case TonePreviewState::Played:
        return L"Toque de teste enviado ao Windows. Sem chamada SIP.";
    case TonePreviewState::MissingDevice:
        return L"Saida do toque desconectada. Atualize dispositivos.";
    case TonePreviewState::UnsupportedFormat:
        return L"Formato de audio nao suportado para este teste.";
    case TonePreviewState::Failed:
        return L"Falha no teste de toque. Confira a saida do Windows.";
    case TonePreviewState::Cancelled:
        return L"Teste de toque interrompido. Sem chamada SIP.";
    }
    return L"Teste indisponivel.";
}
} // namespace axv
