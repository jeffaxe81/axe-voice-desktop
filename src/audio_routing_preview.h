#pragma once
#include "audio_selection_feedback.h"
#include <string>
#include <vector>

namespace axv {

// UI-only preferences for the three independent Windows audio roles:
// microphone, call audio and incoming-call ringtone. No Baresip routing.
struct AudioRoutingPreview {
    AudioSelectionRefresh microphone;
    AudioSelectionRefresh call_output;
    AudioSelectionRefresh ringtone_output;

    bool ready_to_preview() const {
        return microphone.available() && call_output.available() &&
               ringtone_output.available();
    }
    bool separate_outputs() const {
        return call_output.available() && ringtone_output.available() &&
               call_output.selected_id != ringtone_output.selected_id;
    }
};

// Ringing follows the multimedia default when the user has not selected a
// particular endpoint. This may differ from the communication default.
// On hotplug the preference falls back without keeping the removed device ID.
inline AudioSelectionRefresh refresh_ringtone_selection(
    const std::vector<AudioEndpoint>& outputs, const std::wstring& previous_id) {
    if (outputs.empty()) return {};
    int index = -1;
    if (!previous_id.empty()) {
        for (std::size_t i = 0; i < outputs.size(); ++i) {
            if (outputs[i].id == previous_id) {
                index = static_cast<int>(i);
                break;
            }
        }
    }
    if (index < 0) {
        for (std::size_t i = 0; i < outputs.size(); ++i) {
            if (outputs[i].default_multimedia) {
                index = static_cast<int>(i);
                break;
            }
        }
    }
    if (index < 0) index = preferred_audio_endpoint_index(outputs, {});
    if (index < 0) return {};
    const auto& chosen = outputs[static_cast<std::size_t>(index)];
    return {index, chosen.id, !previous_id.empty() && previous_id != chosen.id};
}

inline AudioRoutingPreview preview_audio_routing(
    const std::vector<AudioEndpoint>& inputs,
    const std::vector<AudioEndpoint>& outputs,
    const std::wstring& previous_microphone,
    const std::wstring& previous_call_output,
    const std::wstring& previous_ringtone_output) {
    return {
        refresh_audio_selection(inputs, previous_microphone),
        refresh_audio_selection(outputs, previous_call_output),
        refresh_ringtone_selection(outputs, previous_ringtone_output)
    };
}

inline const wchar_t* audio_routing_preview_feedback(
    const AudioRoutingPreview& preview) {
    if (!preview.microphone.available())
        return L"Microfone indisponivel; preferencias locais. Sem audio SIP.";
    if (!preview.call_output.available())
        return L"Saida da conversa indisponivel; sem audio SIP.";
    if (!preview.ringtone_output.available())
        return L"Saida do toque indisponivel; sem audio SIP.";
    if (preview.separate_outputs())
        return L"Toque e conversa em saidas diferentes; somente pre-visualizacao, sem audio SIP.";
    return L"Toque e conversa na mesma saida; somente pre-visualizacao, sem audio SIP.";
}

} // namespace axv
