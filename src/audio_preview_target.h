#pragma once
#include "audio_devices.h"
#include <string>
#include <vector>

namespace axv {
// Local test tones only: no SIP, microphone activation, device-default changes,
// or Baresip media configuration. Never fall back silently to another speaker.
enum class AudioPreviewRole { Voice, Ringtone };

struct AudioPreviewTarget {
    bool available = false;
    std::wstring endpoint_id;
};

inline AudioPreviewTarget select_audio_preview_target(
    const std::vector<AudioEndpoint>& outputs,
    const std::wstring& voice_id, const std::wstring& ringtone_id,
    AudioPreviewRole role) {
    const auto& selected = role == AudioPreviewRole::Voice ? voice_id : ringtone_id;
    if (selected.empty() || selected.size() > 4096) return {};
    for (const auto& output : outputs) {
        if (!output.id.empty() && output.id == selected)
            return {true, output.id};
    }
    return {};
}
} // namespace axv
