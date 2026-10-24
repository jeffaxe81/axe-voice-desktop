#pragma once
#include "audio_selection_feedback.h"
#include <utility>

namespace axv {

// Presentation-only routing intentions, not a Baresip audio backend.
// Never persist or print Core Audio endpoint IDs. Device names are not trusted
// to imply "computer" or "headset"; the operator chooses the actual endpoints.
struct AudioOutputRouting {
    AudioSelectionRefresh voice;
    AudioSelectionRefresh ring;
    bool independent_outputs() const {
        return voice.available() && ring.available() &&
               voice.selected_id != ring.selected_id;
    }
};

inline AudioOutputRouting refresh_audio_output_routing(
        const std::vector<AudioEndpoint>& outputs,
        const std::wstring& previous_voice,
        const std::wstring& previous_ring) {
    AudioOutputRouting result;
    result.voice = refresh_audio_selection(outputs, previous_voice);
    if (!result.voice.available()) return result;

    result.ring = refresh_audio_selection(outputs, previous_ring);
    // First use, or unplugged ringing endpoint: prefer a *different* output.
    // The operator may still intentionally select the same output for both.
    if (previous_ring.empty() || result.ring.replaced_missing_device) {
        for (std::size_t i = 0; i < outputs.size(); ++i) {
            if (outputs[i].id != result.voice.selected_id) {
                result.ring = {static_cast<int>(i), outputs[i].id,
                               !previous_ring.empty() && previous_ring != outputs[i].id};
                break;
            }
        }
    }
    return result;
}

inline bool can_swap_audio_outputs(const AudioOutputRouting& route) {
    return route.independent_outputs();
}

inline std::pair<std::wstring, std::wstring> swap_audio_output_roles(
        const AudioOutputRouting& route) {
    // First is voice, second is ringing. With one endpoint, retain both.
    if (!can_swap_audio_outputs(route))
        return {route.voice.selected_id, route.ring.selected_id};
    return {route.ring.selected_id, route.voice.selected_id};
}

inline const wchar_t* audio_routing_feedback(
        const AudioSelectionRefresh& microphone,
        const AudioOutputRouting& route) {
    if (!microphone.available()) return L"Microfone ausente. Toque e voz nao estao aplicados ao Baresip.";
    if (!route.voice.available() || !route.ring.available())
        return L"Saida de audio ausente. Toque e voz indisponiveis.";
    if (route.voice.replaced_missing_device || route.ring.replaced_missing_device)
        return L"Dispositivo removido: rota local substituida. Baresip nao configurado.";
    if (route.independent_outputs())
        return L"Toque e voz em saidas distintas (preparado, ainda sem SIP).";
    return L"Toque e voz na mesma saida (preparado, ainda sem SIP).";
}
} // namespace axv
