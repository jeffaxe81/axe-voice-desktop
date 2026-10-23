#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace axv {

// Fixed, deliberately quiet 660 Hz preview beep (250 ms maximum). These
// samples are never copied to/from the microphone and do not involve SIP.
enum class ToneSampleType { Float32, Pcm16, Pcm32 };
struct ToneFormat {
    std::uint32_t sample_rate = 0;
    std::uint16_t channels = 0;
    ToneSampleType type = ToneSampleType::Float32;
};
inline std::size_t tone_sample_bytes(ToneSampleType type) {
    return type == ToneSampleType::Pcm16 ? 2u : 4u;
}
inline bool tone_format_valid(ToneFormat format) {
    return format.sample_rate >= 8000 && format.sample_rate <= 192000 &&
           format.channels > 0 && format.channels <= 8;
}
inline std::uint32_t tone_total_frames(ToneFormat format) {
    return tone_format_valid(format) ? format.sample_rate / 4u : 0u;
}
// Writes only inside the caller-provided packet. Zero (silence) at both
// boundaries and 10ms cosine ramps avoid clicks on connect/disconnect.
inline bool write_tone_packet(unsigned char* output, std::size_t capacity,
                              ToneFormat format, std::uint32_t first_frame,
                              std::uint32_t count) {
    if (!output || !tone_format_valid(format)) return false;
    const auto total = tone_total_frames(format);
    if (first_frame > total || count > total - first_frame) return false;
    const auto sample_bytes = tone_sample_bytes(format.type);
    const auto stride = sample_bytes * static_cast<std::size_t>(format.channels);
    if (count > capacity / stride) return false;
    constexpr double kPi = 3.14159265358979323846;
    const double ramp = static_cast<double>(format.sample_rate) / 100.0;
    for (std::uint32_t frame = 0; frame < count; ++frame) {
        const auto index = first_frame + frame;
        const auto remaining = total - 1u - index;
        const double fade_in = ramp > 0 ? std::min(1.0, index / ramp) : 0.0;
        const double fade_out = ramp > 0 ? std::min(1.0, remaining / ramp) : 0.0;
        const double gain = 0.08 * std::min(fade_in, fade_out);
        const double sample = gain * std::sin(2.0 * kPi * 660.0 *
                                        static_cast<double>(index) / format.sample_rate);
        for (std::uint16_t ch = 0; ch < format.channels; ++ch) {
            auto* ptr = output + static_cast<std::size_t>(frame) * stride +
                        static_cast<std::size_t>(ch) * sample_bytes;
            if (format.type == ToneSampleType::Float32) {
                const float value = static_cast<float>(sample);
                std::memcpy(ptr, &value, sizeof(value));
            } else if (format.type == ToneSampleType::Pcm16) {
                const auto value = static_cast<std::int16_t>(sample * 32767.0);
                std::memcpy(ptr, &value, sizeof(value));
            } else {
                const auto value = static_cast<std::int32_t>(sample * 2147483647.0);
                std::memcpy(ptr, &value, sizeof(value));
            }
        }
    }
    return true;
}
} // namespace axv
