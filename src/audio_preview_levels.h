#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace axv {
// Controls amplitude of the short LOCAL test tone only; not Windows,
// microphone, Baresip, SIP media or a hardware volume control.
inline constexpr std::array<std::uint8_t, 4> kPreviewLevels{{25, 50, 75, 100}};
inline constexpr std::uint8_t kPreviewDefaultLevel = 75;
inline int preview_level_index(std::uint8_t level) {
    for (std::size_t i = 0; i < kPreviewLevels.size(); ++i)
        if (kPreviewLevels[i] == level) return static_cast<int>(i);
    return -1;
}
inline std::uint8_t preview_level_for_index(int index, std::uint8_t fallback) {
    if (index < 0 || static_cast<std::size_t>(index) >= kPreviewLevels.size())
        return fallback;
    return kPreviewLevels[static_cast<std::size_t>(index)];
}
inline bool preview_level_valid(unsigned level) { return level <= 100; }
inline float preview_scale(float sample, std::uint8_t level) {
    return preview_level_valid(level) ? sample * (static_cast<float>(level) / 100.f) : 0.f;
}
} // namespace axv
