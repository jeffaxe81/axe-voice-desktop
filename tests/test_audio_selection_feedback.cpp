#include "../src/audio_selection_feedback.h"
#include <string_view>
#include <vector>

int main() {
    const std::vector<axv::AudioEndpoint> inputs{
        {L"mic-old", L"Microfone USB", false},
        {L"mic-default", L"Microfone integrado", true},
    };
    const std::vector<axv::AudioEndpoint> outputs{
        {L"speaker-old", L"Fone USB", false},
        {L"speaker-default", L"Alto-falante", true},
    };
    const auto first_input = axv::refresh_audio_selection(inputs, L"");
    const auto first_output = axv::refresh_audio_selection(outputs, L"");
    if (first_input.index != 1 || first_input.selected_id != L"mic-default" ||
        first_input.replaced_missing_device) return 1;
    if (first_output.index != 1 || first_output.replaced_missing_device) return 2;
    const auto retained_input = axv::refresh_audio_selection(inputs, L"mic-old");
    const auto retained_output = axv::refresh_audio_selection(outputs, L"speaker-old");
    if (retained_input.index != 0 || retained_input.replaced_missing_device ||
        retained_output.index != 0 || retained_output.replaced_missing_device) return 3;
    if (std::wstring_view(axv::audio_selection_feedback(retained_input, retained_output))
            .find(L"Baresip") == std::wstring_view::npos) return 4;
    const auto recovered_input = axv::refresh_audio_selection(inputs, L"mic-removed");
    const auto recovered_output = axv::refresh_audio_selection(outputs, L"speaker-removed");
    if (recovered_input.index != 1 || !recovered_input.replaced_missing_device ||
        recovered_output.index != 1 || !recovered_output.replaced_missing_device) return 5;
    if (std::wstring_view(axv::audio_selection_feedback(recovered_input, retained_output))
            .find(L"Microfone desconectado") == std::wstring_view::npos) return 6;
    if (std::wstring_view(axv::audio_selection_feedback(retained_input, recovered_output))
            .find(L"Saida desconectada") == std::wstring_view::npos) return 7;
    if (std::wstring_view(axv::audio_selection_feedback(recovered_input, recovered_output))
            .find(L"reassociadas") == std::wstring_view::npos) return 8;
    const auto empty = axv::refresh_audio_selection({}, L"removed");
    if (empty.available() || !empty.selected_id.empty() ||
        empty.replaced_missing_device) return 9;
    if (std::wstring_view(axv::audio_selection_feedback(empty, first_output))
            .find(L"Microfone ausente") == std::wstring_view::npos) return 10;
    if (std::wstring_view(axv::audio_selection_feedback(first_input, empty))
            .find(L"Saida de audio ausente") == std::wstring_view::npos) return 11;
    if (std::wstring_view(axv::audio_selection_feedback(empty, empty))
            .find(L"Sem microfone") == std::wstring_view::npos) return 12;
    if (std::wstring_view(axv::audio_selection_feedback(recovered_input, recovered_output))
            .find(L"mic-default") != std::wstring_view::npos) return 13;
    return 0;
}
