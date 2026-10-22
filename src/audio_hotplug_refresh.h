#pragma once
#include "audio_devices.h"

namespace axv {

// Compare only the Windows endpoint inventory: never inspect SIP account data.
// IDs are ephemeral and stay in memory. This does not open audio streams.
inline bool same_audio_endpoints(const std::vector<AudioEndpoint>& a,
                                 const std::vector<AudioEndpoint>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id ||
            a[i].name != b[i].name ||
            a[i].default_communications != b[i].default_communications)
            return false;
    }
    return true;
}

// Avoid dropping and recreating combo rows on unchanged periodic polls.
// A changed default can matter even if the endpoint IDs have not changed.
inline bool audio_inventory_changed(const AudioInventory& old_snapshot,
                                    const AudioInventory& new_snapshot) {
    return old_snapshot.error != new_snapshot.error ||
        old_snapshot.input_count != new_snapshot.input_count ||
        old_snapshot.output_count != new_snapshot.output_count ||
        old_snapshot.has_default_communications_input != new_snapshot.has_default_communications_input ||
        old_snapshot.has_default_communications_output != new_snapshot.has_default_communications_output ||
        !same_audio_endpoints(old_snapshot.input_devices, new_snapshot.input_devices) ||
        !same_audio_endpoints(old_snapshot.output_devices, new_snapshot.output_devices);
}

} // namespace axv
