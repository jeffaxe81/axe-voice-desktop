#include "../src/ringtone_preview.h"
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
    if (s16[rate / 4] == 0) return 7;
    if (s16[rate * 35 / 100] != 0) return 8;
    if (render_ring_samples(nullptr, frames, 2, rate, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), 0, 2, rate, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), frames, 9, rate, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), frames, 2, 0, RingSampleFormat::Float32) ||
        render_ring_samples(left_right.data(), frames, 2, 1000000, RingSampleFormat::Float32))
        return 9;
    return 0;
}
