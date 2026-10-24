#pragma once
#include <atomic>
#include "audio_preview_levels.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace axv {
// A short, non-looping local speaker test; NOT a SIP incoming-call ringtone.
enum class RingSampleFormat { Float32, Pcm16 };
enum class RingPreviewResult {
    Played, DeviceUnavailable, UnsupportedFormat, AudioServiceUnavailable, Cancelled,
    InvalidLevel
};

inline float ring_sample(std::uint64_t frame, std::uint32_t sample_rate) {
    if (sample_rate < 8000 || sample_rate > 192000) return 0.f;
    const double time = static_cast<double>(frame) / sample_rate;
    double elapsed = -1.0;
    if (time < 0.12) elapsed = time;
    else if (time >= 0.20 && time < 0.32) elapsed = time - 0.20;
    if (elapsed < 0.) return 0.f;
    // 8ms fade-in/out prevents sharp clicks; low amplitude (<= 11%).
    double envelope = elapsed / 0.008;
    if (envelope > 1.) envelope = 1.;
    const double release = (0.12 - elapsed) / 0.008;
    if (release < envelope) envelope = release;
    if (envelope < 0.) envelope = 0.;
    constexpr double two_pi = 6.2831853071795864769;
    const double phase = two_pi * 660.0 * time;
    return static_cast<float>(0.11 * envelope * std::sin(phase));
}

// Pure, bounded PCM generator; a deterministic test can exercise this without
// opening a sound device. The caller owns a correctly sized/aligned buffer.
inline bool render_ring_samples(void* target, std::size_t frames,
                                unsigned channels, std::uint32_t rate,
                                RingSampleFormat encoding,
                                std::uint8_t level_percent = 100) {
    if (!target || !frames || channels == 0 || channels > 8 ||
        rate < 8000 || rate > 192000 || !preview_level_valid(level_percent) ||
        frames > static_cast<std::size_t>(rate) * 2 ||
        frames > (std::numeric_limits<std::size_t>::max)() / channels)
        return false;
    const auto count = frames * channels;
    if (encoding == RingSampleFormat::Float32) {
        auto* samples = static_cast<float*>(target);
        for (std::size_t i = 0; i < frames; ++i) {
            const float value = preview_scale(ring_sample(i, rate), level_percent);
            for (unsigned channel = 0; channel < channels; ++channel)
                samples[i * channels + channel] = value;
        }
        return true;
    }
    if (encoding == RingSampleFormat::Pcm16) {
        auto* samples = static_cast<std::int16_t*>(target);
        for (std::size_t i = 0; i < frames; ++i) {
            const auto value = static_cast<std::int16_t>(
                std::lround(preview_scale(ring_sample(i, rate), level_percent) * 32767.0f));
            for (unsigned channel = 0; channel < channels; ++channel)
                samples[i * channels + channel] = value;
        }
        return true;
    }
    (void)count;
    return false;
}

// Explicit endpoint selection only. Failure does not fall back silently to a
// different speaker and NEVER changes SIP / call / RTP state.
inline bool ring_preview_cancelled(const std::atomic<bool>* signal) {
    return signal && signal->load(std::memory_order_acquire);
}

// Cooperative cancellation; no WASAPI access if already cancelled.
// Signal must outlive the call. The UI joins the worker before destruction.
RingPreviewResult play_selected_ringtone_preview(
    const std::wstring& endpoint_id, const std::atomic<bool>* cancellation = nullptr,
    std::uint8_t level_percent = 100);
} // namespace axv
