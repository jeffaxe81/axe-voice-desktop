#include "../src/audio_split_route.h"
#include <string>
#include <vector>

int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> outputs = {
        {L"headset-secret-id", L"Headset", true},
        {L"speaker-secret-id", L"Computador", false},
        {L"usb-secret-id", L"USB", false}
    };
    const auto voice = refresh_audio_selection(outputs, L"");
    if (voice.index != 0 || voice.selected_id != L"headset-secret-id") return 1;
    const auto initial = refresh_ring_audio_selection(outputs, L"", voice.selected_id);
    if (initial.index != 1 || initial.selected_id != L"speaker-secret-id" ||
        initial.replaced_missing_device) return 2;
    const auto retained = refresh_ring_audio_selection(
        outputs, L"usb-secret-id", voice.selected_id);
    if (retained.index != 2 || retained.replaced_missing_device) return 3;
    const auto same = refresh_ring_audio_selection(
        outputs, voice.selected_id, voice.selected_id);
    if (same.index != 0) return 4;  // intentional shared output is allowed
    std::wstring voice_id = voice.selected_id;
    std::wstring ring_id = initial.selected_id;
    if (!swap_voice_ring_selections(outputs, voice_id, ring_id) ||
        voice_id != L"speaker-secret-id" || ring_id != L"headset-secret-id")
        return 5; // ring headset, voice computer
    if (!swap_voice_ring_selections(outputs, voice_id, ring_id) ||
        voice_id != L"headset-secret-id" || ring_id != L"speaker-secret-id")
        return 6; // ring computer, voice headset
    if (swap_voice_ring_selections(outputs, voice_id, voice_id)) return 7;
    std::wstring unknown = L"missing-secret-id";
    if (swap_voice_ring_selections(outputs, voice_id, unknown)) return 8;

    // Ring device removed: revert to a different active output when possible.
    const std::vector<AudioEndpoint> reduced = {outputs[0], outputs[2]};
    const auto fallback = refresh_ring_audio_selection(
        reduced, L"speaker-secret-id", L"headset-secret-id");
    if (fallback.index != 1 || !fallback.replaced_missing_device) return 9;
    const auto one = refresh_ring_audio_selection({outputs[0]}, L"", voice.selected_id);
    if (one.index != 0 || one.selected_id != voice.selected_id) return 10;
    const auto none = refresh_ring_audio_selection({}, L"ring", voice.selected_id);
    if (none.available() || !none.selected_id.empty()) return 11;
    const auto mixed = voice_ring_preview_feedback(true, voice, initial);
    if (std::wstring(mixed).find(L"separadas") == std::wstring::npos ||
        std::wstring(mixed).find(L"secret-id") != std::wstring::npos ||
        std::wstring(mixed).find(L"Headset") != std::wstring::npos) return 12;
    if (std::wstring(voice_ring_preview_feedback(true, voice, same))
            .find(L"mesma saida") == std::wstring::npos) return 13;
    if (std::wstring(voice_ring_preview_feedback(false, voice, initial))
            .find(L"indisponiveis") == std::wstring::npos) return 14;
    if (std::wstring(voice_ring_preview_feedback(true, voice, fallback))
            .find(L"reassociados") == std::wstring::npos) return 15;
    return 0;
}
