#pragma once
#include "audio_selection_feedback.h"

namespace axv {

// A failed Core Audio scan does not prove that a selected device disappeared.
// Keep preferences in RAM until a successful scan can verify active endpoints.
struct AudioPanelRefreshPlan {
    bool scan_ok = false;
    AudioSelectionRefresh microphone;
    AudioSelectionRefresh speaker;
};

inline AudioPanelRefreshPlan plan_audio_panel_refresh(
    const AudioInventory& inventory,
    const std::wstring& selected_microphone_id,
    const std::wstring& selected_speaker_id) {
    if (inventory.error != AudioInventoryError::None) {
        return {false,
                {-1, selected_microphone_id, false},
                {-1, selected_speaker_id, false}};
    }
    return {true,
            refresh_audio_selection(inventory.input_devices, selected_microphone_id),
            refresh_audio_selection(inventory.output_devices, selected_speaker_id)};
}

} // namespace axv
