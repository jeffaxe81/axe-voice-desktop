#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include "microphone_test.h"
#include "microphone_test_win32.h"
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

MicrophoneTestState capture_once(const std::wstring& endpoint_id,
                                  const std::atomic<bool>* cancel) {
    const auto stopped = [cancel] { return cancel && cancel->load(); };
    if (stopped()) return MicrophoneTestState::Cancelled;
    // The UI must resolve communications default against a fresh inventory;
    // this low-level API never guesses a replacement device.
    if (endpoint_id.empty() || endpoint_id.size() > 4096 ||
        endpoint_id.find(L'\0') != std::wstring::npos)
        return MicrophoneTestState::MissingDevice;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return MicrophoneTestState::Failed;
    const auto capture = [&]() -> MicrophoneTestState {
        ScopedCom<IMMDeviceEnumerator> enumerator;
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator),
                reinterpret_cast<void**>(enumerator.put()))) || !enumerator)
            return MicrophoneTestState::Failed;
        ScopedCom<IMMDevice> device;
        if (FAILED(enumerator->GetDevice(endpoint_id.c_str(), device.put())) || !device)
            return MicrophoneTestState::MissingDevice;
        DWORD state = 0;
        if (FAILED(device->GetState(&state)) || state != DEVICE_STATE_ACTIVE)
            return MicrophoneTestState::MissingDevice;
        ScopedCom<IAudioClient> client;
        if (FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER,
                nullptr, reinterpret_cast<void**>(client.put()))) || !client)
            return MicrophoneTestState::Failed;
        ScopedFormat mix;
        if (FAILED(client->GetMixFormat(&mix.ptr)) || !mix.ptr)
            return MicrophoneTestState::Failed;

        WORD tag = mix.ptr->wFormatTag;
        if (tag == WAVE_FORMAT_EXTENSIBLE) {
            if (mix.ptr->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
                return MicrophoneTestState::UnsupportedFormat;
            const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(mix.ptr);
            if (IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
                tag = WAVE_FORMAT_IEEE_FLOAT;
            else if (IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
                tag = WAVE_FORMAT_PCM;
            else return MicrophoneTestState::UnsupportedFormat;
        }
        MicrophoneSampleType sample_type;
        if (tag == WAVE_FORMAT_IEEE_FLOAT && mix.ptr->wBitsPerSample == 32)
            sample_type = MicrophoneSampleType::Float32;
        else if (tag == WAVE_FORMAT_PCM && mix.ptr->wBitsPerSample == 16)
            sample_type = MicrophoneSampleType::Pcm16;
        else if (tag == WAVE_FORMAT_PCM && mix.ptr->wBitsPerSample == 32)
            sample_type = MicrophoneSampleType::Pcm32;
        else return MicrophoneTestState::UnsupportedFormat;
        MicrophoneSampleFormat format{mix.ptr->nSamplesPerSec, mix.ptr->nChannels, sample_type};
        const std::size_t bytes_per_sample = sample_type == MicrophoneSampleType::Pcm16 ? 2u : 4u;
        if (format.sample_rate < 8000 || format.sample_rate > 192000 ||
            format.channels == 0 || format.channels > 8 ||
            mix.ptr->nBlockAlign != bytes_per_sample * format.channels)
            return MicrophoneTestState::UnsupportedFormat;
        if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                2000000, 0, mix.ptr, nullptr))) return MicrophoneTestState::Failed;
        ScopedCom<IAudioCaptureClient> source;
        if (FAILED(client->GetService(__uuidof(IAudioCaptureClient),
                reinterpret_cast<void**>(source.put()))) || !source)
            return MicrophoneTestState::Failed;
        if (stopped()) return MicrophoneTestState::Cancelled;
        if (FAILED(client->Start())) return MicrophoneTestState::Failed;

        MicrophoneTestState result = MicrophoneTestState::Quiet;
        float peak = 0.0f;
        const ULONGLONG start = GetTickCount64();
        while (GetTickCount64() - start < 850) {
            if (stopped()) { result = MicrophoneTestState::Cancelled; break; }
            UINT32 pending = 0;
            if (FAILED(source->GetNextPacketSize(&pending))) {
                result = MicrophoneTestState::Failed;
                break;
            }
            bool error = false;
            unsigned int handled = 0;
            while (pending && handled++ < 32) {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                if (FAILED(source->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) {
                    error = true;
                    break;
                }
                float packet_peak = 0.0f;
                bool valid = true;
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && frames)
                    valid = microphone_packet_peak(data,
                        static_cast<std::size_t>(frames) * mix.ptr->nBlockAlign,
                        format, frames, packet_peak);
                // Release every acquired buffer, including unsupported data.
                if (FAILED(source->ReleaseBuffer(frames))) error = true;
                if (!valid) { result = MicrophoneTestState::UnsupportedFormat; error = true; }
                peak = std::max(peak, packet_peak);
                if (error || FAILED(source->GetNextPacketSize(&pending))) {
                    error = true;
                    break;
                }
            }
            if (error) {
                if (result != MicrophoneTestState::UnsupportedFormat)
                    result = MicrophoneTestState::Failed;
                break;
            }
            Sleep(10);
        }
        client->Stop();
        if (stopped()) return MicrophoneTestState::Cancelled;
        if (result == MicrophoneTestState::Quiet && peak >= 0.02f)
            return MicrophoneTestState::SignalDetected;
        return result;
    };
    MicrophoneTestState result = MicrophoneTestState::Failed;
    try { result = capture(); } catch (...) { result = MicrophoneTestState::Failed; }
    CoUninitialize();
    return result;
}

} // namespace

MicrophoneTestState test_windows_microphone(const std::wstring& endpoint_id,
                                            const std::atomic<bool>* cancel) {
    return capture_once(endpoint_id, cancel);
}

MicrophoneTestWorker::~MicrophoneTestWorker() {
    request_stop();
    if (worker_.joinable()) worker_.join();
}

bool MicrophoneTestWorker::start(const std::wstring& endpoint_id) {
    if (endpoint_id.empty() || endpoint_id.size() > 4096 ||
        endpoint_id.find(L'\0') != std::wstring::npos ||
        state_.load() == MicrophoneTestState::Capturing) return false;
    if (worker_.joinable()) worker_.join();
    cancel_.store(false);
    state_.store(MicrophoneTestState::Capturing);
    try {
        worker_ = std::thread([this, endpoint_id] {
            MicrophoneTestState result = MicrophoneTestState::Failed;
            try { result = test_windows_microphone(endpoint_id, &cancel_); }
            catch (...) { result = MicrophoneTestState::Failed; }
            state_.store(cancel_.load() ? MicrophoneTestState::Cancelled : result);
        });
    } catch (...) {
        state_.store(MicrophoneTestState::Failed);
        return false;
    }
    return true;
}

const wchar_t* microphone_test_feedback(MicrophoneTestState state) {
    switch (state) {
    case MicrophoneTestState::Idle: return L"Teste local de microfone disponivel.";
    case MicrophoneTestState::Capturing: return L"Escutando microfone por menos de 1 segundo. Nada sera salvo.";
    case MicrophoneTestState::SignalDetected: return L"Microfone: sinal detectado localmente. Sem teste de chamada SIP.";
    case MicrophoneTestState::Quiet: return L"Microfone acessado; sem sinal significativo. Fale perto do headset.";
    case MicrophoneTestState::MissingDevice: return L"Microfone desconectado. Atualize os dispositivos.";
    case MicrophoneTestState::UnsupportedFormat: return L"Formato do microfone nao suportado para este teste.";
    case MicrophoneTestState::Cancelled: return L"Teste de microfone interrompido.";
    case MicrophoneTestState::Failed: return L"Falha de acesso ao microfone; confira privacidade do Windows.";
    }
    return L"Teste de microfone indisponivel.";
}
} // namespace axv
