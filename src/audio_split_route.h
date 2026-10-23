#pragma once
#include "audio_devices.h"
#include "audio_selection_feedback.h"
#include <string>
#include <utility>

namespace axv {

// UI-only preference, never an audio renderer or an RTP routing command.
// First boot: pick an output other than the voice communications default
// for ringing if one exists (often external speakers). No device-type guesses.
inline AudioSelectionRefresh refresh_ring_audio_selection(
    const std::vector<AudioEndpoint>& outputs,
    const std::wstring& previous_ring_id,
    const std::wstring& selected_voice_id) {

    if (outputs.empty()) return {};
    if (!previous_ring_id.empty()) {
        for (std::size_t i = 0; i < outputs.size(); ++i) {
            if (outputs[i].id == previous_ring_id)
                return {static_cast<int>(i), outputs[i].id, false};
        }
    }
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        if (!outputs[i].id.empty() && outputs[i].id != selected_voice_id)
            return {static_cast<int>(i), outputs[i].id,
                    !previous_ring_id.empty()};
    }
    // Only one output (or all choices resolve to the voice output).
    const auto preferred = refresh_audio_selection(outputs, selected_voice_id);
    return {preferred.index, preferred.selected_id,
            !previous_ring_id.empty() && preferred.selected_id != previous_ring_id};
}

inline bool swap_voice_ring_selections(
    const std::vector<AudioEndpoint>& outputs,
    std::wstring& voice_id, std::wstring& ring_id) {
    if (voice_id.empty() || ring_id.empty() || voice_id == ring_id)
        return false;
    bool voice_found = false, ring_found = false;
    for (const auto& endpoint : outputs) {
        if (!endpoint.id.empty() && endpoint.id == voice_id) voice_found = true;
        if (!endpoint.id.empty() && endpoint.id == ring_id) ring_found = true;
    }
    if (!voice_found || !ring_found) return false;
    std::swap(voice_id, ring_id);
    return true;
}

// Never include endpoint IDs or raw Windows-provided names in support text.
inline const wchar_t* voice_ring_preview_feedback(
    bool microphone_available, const AudioSelectionRefresh& voice,
    const AudioSelectionRefresh& ring) {
    if (!microphone_available || !voice.available() || !ring.available())
        return L"Entrada ou saida ausente. Toque e voz SIP indisponiveis.";
    if (voice.selected_id == ring.selected_id)
        return L"Toque e voz na mesma saida (apenas preferencia; sem audio SIP).";
    if (voice.replaced_missing_device || ring.replaced_missing_device)
        return L"Dispositivo alterado: toque e voz reassociados. Sem audio SIP.";
    return L"Saidas separadas para toque e voz (pre-visualizacao; sem audio SIP).";
}
} // namespace axv
