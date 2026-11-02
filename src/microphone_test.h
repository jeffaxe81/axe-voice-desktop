#pragma once
#include "audio_devices.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace axv {

// Explicit Windows capture test only. The worker may inspect PCM frames for
// signal level, but never persists samples, records them, or sends them to SIP.
enum class MicrophoneSampleType { Float32, Pcm16, Pcm32 };
struct MicrophoneSampleFormat {
    std::uint32_t sample_rate = 0;
    std::uint16_t channels = 0;
    MicrophoneSampleType type = MicrophoneSampleType::Float32;
};

inline std::optional<std::wstring> microphone_test_endpoint(
    const std::vector<AudioEndpoint>& inputs,
    const std::wstring& selected_id,
    bool follow_windows_default) {
    if (selected_id.empty() && follow_windows_default) {
        for (const auto& device : inputs)
            if (device.default_communications && !device.id.empty())
                return device.id;
        return std::nullopt;
    }
    if (selected_id.empty()) return std::nullopt;
    for (const auto& device : inputs)
        if (!device.id.empty() && device.id == selected_id) return device.id;
    return std::nullopt; // No silent fallback after device unplug.
}

// Check one WASAPI packet without copying or storing voice samples. Reject
// malformed formats/buffers, NaN/Inf and truncated packets.
inline bool microphone_packet_peak(
    const unsigned char* bytes, std::size_t byte_count,
    MicrophoneSampleFormat format, std::uint32_t frames, float& peak) {
    if (!bytes || format.sample_rate < 8000 || format.sample_rate > 192000 ||
        format.channels == 0 || format.channels > 8 || !frames) return false;
    const std::size_t width = format.type == MicrophoneSampleType::Pcm16 ? 2u : 4u;
    const std::size_t stride = width * format.channels;
    if (frames > byte_count / stride) return false;
    float maximum = 0.0f;
    const std::size_t samples = static_cast<std::size_t>(frames) * format.channels;
    for (std::size_t i = 0; i < samples; ++i) {
        const unsigned char* sample = bytes + i * width;
        float level = 0.0f;
        if (format.type == MicrophoneSampleType::Float32) {
            float value = 0.0f;
            std::memcpy(&value, sample, sizeof(value));
            if (!std::isfinite(value)) return false;
            level = std::fabs(value);
        } else if (format.type == MicrophoneSampleType::Pcm16) {
            std::int16_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            level = std::fabs(static_cast<float>(value) / 32768.0f);
        } else {
            std::int32_t value = 0;
            std::memcpy(&value, sample, sizeof(value));
            level = std::fabs(static_cast<double>(value) / 2147483648.0);
        }
        maximum = std::max(maximum, std::min(level, 1.0f));
    }
    peak = maximum;
    return true;
}

} // namespace axv
