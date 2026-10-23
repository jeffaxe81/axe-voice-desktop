#include "../src/audio_routing_policy.h"
#include <string_view>
#include <vector>

int main() {
    using namespace axv;
    const std::vector<AudioEndpoint> outputs{
        {L"pc-output", L"Speakers (Laptop)", false},
        {L"headset-output", L"Headphones (USB)", true},
    };
    const auto initial = refresh_audio_output_routing(outputs,L"",L"");
    // Default communications endpoint for voice; complementary endpoint for ring.
    if (initial.voice.index != 1 || initial.ring.index != 0 ||
        !initial.independent_outputs()) return 1;
    const auto swapped = swap_audio_output_roles(initial);
    if (swapped.first != L"pc-output" || swapped.second != L"headset-output") return 2;
    const auto reverse = refresh_audio_output_routing(outputs,swapped.first,swapped.second);
    if (reverse.voice.index != 0 || reverse.ring.index != 1 ||
        !can_swap_audio_outputs(reverse)) return 3;
    const auto restored = swap_audio_output_roles(reverse);
    if (restored.first != initial.voice.selected_id ||
        restored.second != initial.ring.selected_id) return 4;

    // Manual same-device choice is respected, with no misleading swap.
    const auto same = refresh_audio_output_routing(outputs,L"headset-output",L"headset-output");
    if (same.independent_outputs() || can_swap_audio_outputs(same)) return 5;
    if (swap_audio_output_roles(same).first != same.voice.selected_id) return 6;

    // Disconnected ringing device falls back to the other available output.
    const auto missing = refresh_audio_output_routing(outputs,L"headset-output",L"unplugged");
    if (missing.ring.selected_id != L"pc-output" ||
        !missing.ring.replaced_missing_device) return 7;

    // Disconnected voice, same ringing ID remains attached to the correct role.
    const auto missing_voice =
        refresh_audio_output_routing(outputs,L"gone",L"pc-output");
    if (missing_voice.voice.selected_id != L"headset-output" ||
        missing_voice.ring.selected_id != L"pc-output") return 8;

    const auto one = refresh_audio_output_routing(
        {{L"one",L"Single output",true}},L"pc-output",L"headset-output");
    if (one.independent_outputs() || can_swap_audio_outputs(one) ||
        one.voice.selected_id != L"one" || one.ring.selected_id != L"one") return 9;

    const auto none = refresh_audio_output_routing({},L"old-voice",L"old-ring");
    if (none.voice.available() || none.ring.available() ||
        !none.voice.selected_id.empty() || !none.ring.selected_id.empty()) return 10;

    // Reordered enumeration must not silently reverse the selection.
    const std::vector<AudioEndpoint> reordered{outputs[1],outputs[0]};
    const auto kept = refresh_audio_output_routing(reordered,
        initial.voice.selected_id, initial.ring.selected_id);
    if (kept.voice.index != 0 || kept.ring.index != 1) return 11;

    const auto mic = refresh_audio_selection(
        {{L"mic",L"Mic",true}},L"");
    if (std::wstring_view(audio_routing_feedback(mic,initial)).find(L"distintas")
        == std::wstring_view::npos) return 12;
    if (std::wstring_view(audio_routing_feedback(mic,same)).find(L"mesma")
        == std::wstring_view::npos) return 13;
    if (std::wstring_view(audio_routing_feedback(mic,missing)).find(L"removido")
        == std::wstring_view::npos) return 14;
    if (std::wstring_view(audio_routing_feedback(mic,none)).find(L"indisponiveis")
        == std::wstring_view::npos) return 15;
    if (std::wstring_view(audio_routing_feedback({},initial)).find(L"Microfone")
        == std::wstring_view::npos) return 16;

    // IDs are never included in diagnostics.
    if (std::wstring_view(audio_routing_feedback(mic,initial)).find(L"headset-output")
        != std::wstring_view::npos) return 17;
    return 0;
}
