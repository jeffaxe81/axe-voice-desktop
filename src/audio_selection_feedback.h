#pragma once
#include "audio_devices.h"

namespace axv {

// A UI-only result. Endpoint IDs remain in memory; never log or persist them.
// This does not prove permissions, configure Baresip or enable SIP/RTP.
struct AudioSelectionRefresh {
    int index = -1;
    std::wstring selected_id;
    bool replaced_missing_device = false;
    bool available() const { return index >= 0; }
};

inline AudioSelectionRefresh refresh_audio_selection(
    const std::vector<AudioEndpoint>& devices, const std::wstring& previous_id) {
    const int index = preferred_audio_endpoint_index(devices, previous_id);
    if (index < 0) return {};
    const auto& selected = devices[static_cast<std::size_t>(index)];
    return {index, selected.id, !previous_id.empty() && selected.id != previous_id};
}

// Always state the lab limitation, even if valid endpoints were discovered.
inline const wchar_t* audio_selection_feedback(
    const AudioSelectionRefresh& microphone, const AudioSelectionRefresh& speaker) {
    if (!microphone.available() && !speaker.available())
        return L"Sem microfone e saida ativos. Audio SIP indisponivel.";
    if (!microphone.available())
        return L"Microfone ausente. Verifique o headset. Sem audio SIP.";
    if (!speaker.available())
        return L"Saida de audio ausente. Verifique o headset. Sem audio SIP.";
    if (microphone.replaced_missing_device && speaker.replaced_missing_device)
        return L"Headset alterado: entrada e saida reassociadas. Sem audio SIP.";
    if (microphone.replaced_missing_device)
        return L"Microfone desconectado: nova entrada selecionada. Sem audio SIP.";
    if (speaker.replaced_missing_device)
        return L"Saida desconectada: nova saida selecionada. Sem audio SIP.";
    return L"Dispositivos selecionados localmente; sem conexao com Baresip.";
}

} // namespace axv
