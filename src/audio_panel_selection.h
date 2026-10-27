#pragma once
#include "audio_routing_preview.h"

namespace axv {

struct AudioPanelSelection {
    std::wstring input_id;
    std::wstring output_id;
    std::wstring ringtone_id;
};

inline AudioPanelSelection reconcile_audio_panel(
    const AudioPanelSelection& previous, const AudioInventory& inventory,
    bool native_routes) {
    // A failed query cannot establish that an endpoint was removed. Native
    // routes also preserve explicit Windows defaults (empty IDs) and missing IDs.
    if (inventory.error != AudioInventoryError::None || native_routes)
        return previous;
    return {
        refresh_audio_selection(inventory.input_devices, previous.input_id).selected_id,
        refresh_audio_selection(inventory.output_devices, previous.output_id).selected_id,
        refresh_ringtone_selection(inventory.output_devices, previous.ringtone_id).selected_id
    };
}

} // namespace axv
