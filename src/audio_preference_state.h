#pragma once

#include "audio_devices.h"
#include <cstddef>
#include <string>
#include <vector>

namespace axv {

// Entirely local UI preference; never stored on disk or sent to the SIP engine.
// Explicit choice survives temporary device removal and Core Audio query errors.
struct AudioChoiceResolution {
    int index = -1;
    bool preferred_unavailable = false;
    bool preferred_restored = false;
    bool automatic_switch = false;
    bool available() const { return index >= 0; }
};

class AudioDeviceChoice {
public:
    void choose(const std::vector<AudioEndpoint>& devices, std::size_t index) {
        if (index >= devices.size()) return;
        preferred_id_ = devices[index].id;
        active_id_ = preferred_id_;
    }

    AudioChoiceResolution reconcile(const std::vector<AudioEndpoint>& devices) {
        const std::wstring previous = active_id_;
        int index = -1;
        if (!preferred_id_.empty()) {
            for (std::size_t i = 0; i < devices.size(); ++i) {
                if (devices[i].id == preferred_id_) {
                    index = static_cast<int>(i);
                    break;
                }
            }
        }
        if (index < 0) index = preferred_audio_endpoint_index(devices, active_id_);
        active_id_ = index < 0 ? L"" : devices[static_cast<std::size_t>(index)].id;
        const bool missing = !preferred_id_.empty() && active_id_ != preferred_id_;
        const bool restored = !preferred_id_.empty() && !missing && previous != active_id_;
        const bool switched = !previous.empty() && previous != active_id_ && !restored;
        return {index, missing, restored, switched};
    }

    // Used for assertions; endpoint identity is never included in UI messages.
    const std::wstring& active_id() const { return active_id_; }
    const std::wstring& preferred_id() const { return preferred_id_; }
private:
    std::wstring active_id_;
    std::wstring preferred_id_;
};

// Never claim Baresip audio or SIP registration based on Windows endpoints.
inline const wchar_t* audio_choice_feedback(
    const AudioChoiceResolution& input, const AudioChoiceResolution& output) {
    if (!input.available() && !output.available())
        return L"Sem microfone e saida ativos. Audio SIP indisponivel.";
    if (!input.available())
        return L"Microfone ausente. Verifique o headset. Sem audio SIP.";
    if (!output.available())
        return L"Saida de audio ausente. Verifique o headset. Sem audio SIP.";
    if (input.preferred_restored || output.preferred_restored)
        return L"Headset reconectado: preferencia restaurada. Sem audio SIP.";
    if (input.preferred_unavailable || output.preferred_unavailable)
        return L"Headset preferido ausente: usando alternativa. Sem audio SIP.";
    if (input.automatic_switch || output.automatic_switch)
        return L"Dispositivo alterado automaticamente. Sem audio SIP.";
    return L"Dispositivos selecionados localmente; sem conexao com Baresip.";
}

} // namespace axv
