#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace axv {
// Pure, deterministic short chime. No SIP, device identifiers or side effects.
enum class ToneSampleEncoding { Float32, Signed16 };
inline float preview_tone_sample(std::uint64_t frame,
                                 std::uint32_t sample_rate,
                                 std::uint32_t duration_ms = 240) {
    if (sample_rate < 8000 || sample_rate > 192000 ||
        duration_ms == 0 || duration_ms > 500) return 0.0f;
    const std::uint64_t length = (static_cast<std::uint64_t>(sample_rate) * duration_ms) / 1000u;
    if (!length || frame >= length) return 0.0f;
    const std::uint64_t ramp = std::max<std::uint64_t>(1, sample_rate / 125u);
    const double fade_in = std::min(1.0, static_cast<double>(frame) / ramp);
    const double fade_out = std::min(1.0, static_cast<double>(length - frame - 1) / ramp);
    const double envelope = std::min(fade_in, fade_out);
    constexpr double tau = 6.2831853071795864769;
    const double frequency_hz = 880.0; // deliberately short, modest volume
    return static_cast<float>(0.11 * envelope *
        std::sin(tau * frequency_hz * static_cast<double>(frame) / sample_rate));
}
inline bool fill_preview_tone(void* buffer, std::uint32_t frames,
                              std::uint16_t channels, std::uint32_t sample_rate,
                              ToneSampleEncoding encoding) {
    if (!buffer || frames == 0 || frames > 384000 || channels == 0 ||
        channels > 8 || sample_rate < 8000 || sample_rate > 192000) return false;
    if (encoding == ToneSampleEncoding::Float32) {
        auto* dst = static_cast<float*>(buffer);
        for (std::uint32_t i = 0; i < frames; ++i) {
            const float sample = preview_tone_sample(i, sample_rate);
            for (std::uint16_t c = 0; c < channels; ++c)
                dst[static_cast<std::size_t>(i) * channels + c] = sample;
        }
        return true;
    }
    if (encoding == ToneSampleEncoding::Signed16) {
        auto* dst = static_cast<std::int16_t*>(buffer);
        for (std::uint32_t i = 0; i < frames; ++i) {
            const auto sample = static_cast<std::int16_t>(
                preview_tone_sample(i, sample_rate) * 32767.0f);
            for (std::uint16_t c = 0; c < channels; ++c)
                dst[static_cast<std::size_t>(i) * channels + c] = sample;
        }
        return true;
    }
    return false;
}
} // namespace axv
