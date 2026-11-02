#pragma once
#include "audio_devices.h"
#include <optional>
#include <string>
#include <vector>

namespace axv {

// Resolve the exact endpoint that will receive a LOCAL Windows test tone.
// A Windows-default row is resolved from the *current inventory* only; an
// explicitly chosen but unplugged endpoint must never be silently replaced.
// This does not modify the actual Baresip audio route or verify SIP media.
enum class AudioOutputPreviewRole { Conversation, Ringtone };

inline std::optional<std::wstring> audio_output_preview_endpoint(
    const std::vector<AudioEndpoint>& outputs,
    const std::wstring& selected_id,
    bool follow_windows_default,
    AudioOutputPreviewRole role) {
    if (follow_windows_default && selected_id.empty()) {
        for (const auto& endpoint : outputs) {
            if (!endpoint.id.empty() &&
                (role == AudioOutputPreviewRole::Conversation
                    ? endpoint.default_communications
                    : endpoint.default_multimedia))
                return endpoint.id;
        }
        return std::nullopt; // No verified default: do not guess another.
    }
    if (selected_id.empty()) return std::nullopt;
    for (const auto& endpoint : outputs)
        if (endpoint.id == selected_id) return endpoint.id;
    return std::nullopt;
}

} // namespace axv
