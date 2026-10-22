#include "../src/audio_preference_state.h"
#include <string_view>
#include <vector>

int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> input {
        {L"usb-input", L"Headset USB", false},
        {L"integrated", L"Microfone padrao", true}
    };
    const std::vector<AudioEndpoint> output {
        {L"usb-output", L"Fone USB", false},
        {L"speakers", L"Alto falante padrao", true}
    };
    AudioDeviceChoice mic, speaker;
    auto m = mic.reconcile(input);
    auto s = speaker.reconcile(output);
    if (m.index != 1 || s.index != 1 || m.preferred_unavailable ||
        m.preferred_restored || m.automatic_switch) return 1;

    // Explicit selection is not erased by an automatic fallback.
    mic.choose(input, 0);
    speaker.choose(output, 0);
    m = mic.reconcile(input); s = speaker.reconcile(output);
    if (m.index != 0 || s.index != 0 || m.preferred_restored) return 2;
    std::vector<AudioEndpoint> without_mic{input[1]};
    std::vector<AudioEndpoint> without_speaker{output[1]};
    m = mic.reconcile(without_mic); s = speaker.reconcile(without_speaker);
    if (m.index != 0 || !m.preferred_unavailable || !s.preferred_unavailable ||
        mic.active_id() != L"integrated" || mic.preferred_id() != L"usb-input") return 3;
    if (std::wstring_view(audio_choice_feedback(m, s)).find(L"alternativa") ==
        std::wstring_view::npos) return 4;

    // No device identity escapes through the diagnostic.
    if (std::wstring_view(audio_choice_feedback(m, s)).find(L"usb-input") !=
        std::wstring_view::npos) return 5;

    // A transient enumeration error bypasses reconcile: both IDs survive.
    const auto chosen = mic.preferred_id();
    const auto fallback = mic.active_id();
    if (chosen != L"usb-input" || fallback != L"integrated") return 6;
    m = mic.reconcile(input); s = speaker.reconcile(output);
    if (m.index != 0 || !m.preferred_restored || !s.preferred_restored ||
        mic.active_id() != L"usb-input") return 7;
    if (std::wstring_view(audio_choice_feedback(m, s)).find(L"reconectado") ==
        std::wstring_view::npos) return 8;
    m = mic.reconcile(input);
    if (m.preferred_restored || m.automatic_switch || m.preferred_unavailable) return 9;

    // The user can override a prior preference while using a fallback.
    mic.reconcile(without_mic);
    mic.choose(without_mic, 0);
    m = mic.reconcile(input);
    if (m.index != 1 || mic.preferred_id() != L"integrated" ||
        m.preferred_restored || m.preferred_unavailable) return 10;
    mic.choose(input, 500);  // Invalid UI index is ignored.
    if (mic.preferred_id() != L"integrated") return 11;

    // No connected endpoint is never presented as a working audio route.
    speaker.choose(output, 0);
    s = speaker.reconcile({});
    if (s.available() || !s.preferred_unavailable || !speaker.active_id().empty()) return 12;
    s = speaker.reconcile(output);
    if (!s.preferred_restored || s.index != 0) return 13;
    AudioDeviceChoice unknown;
    m = unknown.reconcile({});
    if (m.available() || m.preferred_unavailable || m.preferred_restored) return 14;
    if (std::wstring_view(audio_choice_feedback(m, s)).find(L"Microfone ausente") ==
        std::wstring_view::npos) return 15;
    return 0;
}
