#pragma once
#include <cstdint>
namespace axv {
inline constexpr unsigned kBaseDpi = 96;
inline constexpr unsigned normalized_dpi(unsigned dpi) {
    return dpi < 96 ? 96u : (dpi > 384 ? 384u : dpi);
}
// Original logical dimensions are always in 96-DPI units, not pre-scaled pixels.
inline constexpr int dpi_pixels(int logical, unsigned dpi) {
    const auto current = normalized_dpi(dpi);
    if (logical >= 0)
        return static_cast<int>((static_cast<std::int64_t>(logical)*current + 48) / 96);
    return -dpi_pixels(-logical, current);
}
struct DpiBox { int x, y, width, height; };
inline constexpr DpiBox scaled_box(DpiBox b, unsigned dpi) {
    return {dpi_pixels(b.x,dpi), dpi_pixels(b.y,dpi),
            dpi_pixels(b.width,dpi), dpi_pixels(b.height,dpi)};
}
} // namespace axv
