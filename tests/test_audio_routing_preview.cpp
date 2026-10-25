#include "../src/audio_routing_preview.h"
#include <string_view>
#include <vector>

int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> microphones{
        {L"usb-mic", L"Headset mic", true},
        {L"built-in-mic", L"Laptop mic", false}
    };
    const std::vector<AudioEndpoint> outputs{
        {L"usb-headset", L"Headset", true, false},
        {L"laptop-speakers", L"Computer", false, true}
    };

    // Default call audio = communication endpoint; ringtone = multimedia.
    const auto standard = preview_audio_routing(microphones, outputs, L"", L"", L"");
    if (!standard.ready_to_preview() || !standard.separate_outputs()) return 1;
    if (standard.microphone.selected_id != L"usb-mic" ||
        standard.call_output.selected_id != L"usb-headset" ||
        standard.ringtone_output.selected_id != L"laptop-speakers") return 2;
    if (std::wstring_view(audio_routing_preview_feedback(standard))
         .find(L"saidas diferentes") == std::wstring_view::npos) return 3;

    // Explicit inversion: conversation through computer, ring through headset.
    const auto inverted = preview_audio_routing(
        microphones, outputs, L"built-in-mic", L"laptop-speakers", L"usb-headset");
    if (!inverted.ready_to_preview() || !inverted.separate_outputs() ||
        inverted.call_output.selected_id != L"laptop-speakers" ||
        inverted.ringtone_output.selected_id != L"usb-headset") return 4;

    // Both roles can intentionally use the same physical output device.
    const auto same = preview_audio_routing(
        microphones, outputs, L"usb-mic", L"usb-headset", L"usb-headset");
    if (same.separate_outputs() || !same.ready_to_preview()) return 5;
    if (std::wstring_view(audio_routing_preview_feedback(same))
        .find(L"mesma saida") == std::wstring_view::npos) return 6;

    // Preserve the explicit preference on refresh, even if default role changes.
    auto changed = outputs;
    changed[0].default_multimedia = true;
    changed[1].default_multimedia = false;
    const auto retained = preview_audio_routing(
        microphones, changed, L"usb-mic", L"usb-headset", L"laptop-speakers");
    if (retained.ringtone_output.index != 1 ||
        retained.ringtone_output.replaced_missing_device) return 7;

    // Hotplug: detached ringtone output falls back to current multimedia default.
    changed.erase(changed.begin() + 1);
    const auto detached = preview_audio_routing(
        microphones, changed, L"usb-mic", L"usb-headset", L"laptop-speakers");
    if (!detached.ready_to_preview() ||
        !detached.ringtone_output.replaced_missing_device ||
        detached.ringtone_output.selected_id != L"usb-headset" ||
        detached.separate_outputs()) return 8;
    if (!detached.ringtone_output.selected_id.empty() &&
        detached.ringtone_output.selected_id == L"laptop-speakers") return 9;

    // Fall back to communications when Windows has no multimedia default.
    std::vector<AudioEndpoint> no_multimedia{
        {L"speaker", L"Speaker", false, false},
        {L"headphones", L"Headphones", true, false}
    };
    const auto fallback = refresh_ringtone_selection(no_multimedia, L"gone");
    if (!fallback.available() || fallback.index != 1 ||
        !fallback.replaced_missing_device) return 10;

    // Without any designated default, use first enumerated active endpoint.
    no_multimedia[1].default_communications = false;
    if (refresh_ringtone_selection(no_multimedia, L"").index != 0) return 11;

    // Missing devices never retain removed endpoint IDs or claim audio SIP.
    if (refresh_ringtone_selection({}, L"gone").available()) return 12;
    const auto no_out = preview_audio_routing(microphones, {}, L"", L"gone", L"gone");
    if (no_out.ready_to_preview() || no_out.call_output.available() ||
        no_out.ringtone_output.available() ||
        std::wstring_view(audio_routing_preview_feedback(no_out))
            .find(L"sem audio SIP") == std::wstring_view::npos) return 13;
    const auto no_microphone = preview_audio_routing({}, outputs, L"old", L"", L"");
    if (no_microphone.ready_to_preview() ||
        std::wstring_view(audio_routing_preview_feedback(no_microphone))
            .find(L"Microfone indisponivel") == std::wstring_view::npos) return 14;
    return 0;
}
