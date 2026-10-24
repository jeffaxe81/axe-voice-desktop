#include "../src/ringtone_preview.h"
#include <array>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
int main() {
    using namespace axv;
    constexpr std::uint32_t rate = 48000;
    if (ring_sample(100, 0) != 0.f ||
        ring_sample(rate / 2, rate) != 0.f ||
        ring_sample(rate * 3 / 20, rate) != 0.f) return 1;
    double first_peak = 0., second_peak = 0., silence_peak = 0.;
    for (std::size_t i = 0; i < rate * 4 / 10; ++i) {
        const double a = std::fabs(ring_sample(i, rate));
        if (a > 0.111) return 2; // low listening level
        if (i < rate * 12 / 100 && a > first_peak) first_peak = a;
        if (i >= rate * 20 / 100 && i < rate * 32 / 100 &&
            a > second_peak) second_peak = a;
        if (i >= rate * 12 / 100 && i < rate * 20 / 100 &&
            a > silence_peak) silence_peak = a;
    }
    if (first_peak < .09 || second_peak < .09 || silence_peak != 0.) return 3;
    constexpr std::size_t frames = rate * 4 / 10;
    std::vector<float> left_right(frames * 2);
    if (!render_ring_samples(left_right.data(), frames, 2, rate,
                             RingSampleFormat::Float32)) return 4;
    for (std::size_t i = 0; i < frames; ++i) {
        if (left_right[i * 2] != left_right[i * 2 + 1]) return 5;
    }
    std::vector<std::int16_t> s16(frames);
    if (!render_ring_samples(s16.data(), frames, 1, rate,
                             RingSampleFormat::Pcm16)) return 6;
    if (s16[rate * 205 / 1000] == 0) return 7;
    if (s16[rate * 35 / 100] != 0) return 8;
    if (render_ring_samples(nullptr, frames, 2, rate, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), 0, 2, rate, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), frames, 9, rate, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), frames, 2, 0, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), frames, 2, 1000000, RingSampleFormat::Float32))
        return 9;
    // Must cancel before opening a real endpoint/COM; CI remains hardware free.
    std::atomic<bool> cancel{true};
    if (!ring_preview_cancelled(&cancel) ||
        play_selected_ringtone_preview(L"not-a-real-device", &cancel) !=
            RingPreviewResult::Cancelled) return 10;
    cancel.store(false);
    if (ring_preview_cancelled(&cancel) || ring_preview_cancelled(nullptr)) return 11;
    cancel.store(true);
    if (play_selected_ringtone_preview(L"", &cancel) != RingPreviewResult::Cancelled) return 12;
    // Unit tests for per-role local level scaling on PCM16/float32.
    if (preview_level_index(25) != 0 || preview_level_index(50) != 1 ||
        preview_level_index(75) != 2 || preview_level_index(100) != 3 ||
        preview_level_index(30) != -1 ||
        preview_level_for_index(0, 75) != 25 ||
        preview_level_for_index(3, 75) != 100 ||
        preview_level_for_index(-1, 75) != 75 ||
        preview_level_for_index(4, 75) != 75) return 13;
    std::array<float, 48000> full{};
    std::array<float, 48000> quarter{};
    if (!render_ring_samples(full.data(), 24000, 2, rate,
                             RingSampleFormat::Float32, 100) ||
        !render_ring_samples(quarter.data(), 24000, 2, rate,
                             RingSampleFormat::Float32, 25)) return 14;
    for (std::size_t i=0;i<48000;++i)
        if (std::fabs(quarter[i]-full[i]*0.25f)>0.000001f) return 15;
    std::array<std::int16_t, 48000> half{};
    if (!render_ring_samples(half.data(), 24000, 2, rate,
                             RingSampleFormat::Pcm16, 50)) return 16;
    for (std::size_t i=0;i<48000;++i)
        if (std::abs(static_cast<int>(half[i])-
                     static_cast<int>(std::lround(full[i]*32767.f*0.5f)))>1) return 17;
    if (render_ring_samples(full.data(), 24000, 2, rate,
                            RingSampleFormat::Float32, 101) ||
        play_selected_ringtone_preview(L"fake", nullptr, 101) !=
            RingPreviewResult::InvalidLevel) return 18;
    if (!render_ring_samples(quarter.data(), 24000, 2, rate,
                             RingSampleFormat::Float32, 0)) return 19;
    for (auto value : quarter) if (value != 0.f) return 20;
    return 0;
}
