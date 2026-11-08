#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
namespace axv {
inline constexpr unsigned kBaseDpi = 96;
inline constexpr unsigned normalized_dpi(unsigned dpi) {
    return dpi < 96 ? 96u : (dpi > 384 ? 384u : dpi);
}
// Saturated signed 64-bit DPI arithmetic: INT_MIN and giant logical dimensions
// must not trigger signed overflow or recursive negation of INT_MIN.
inline constexpr int dpi_pixels(int logical, unsigned dpi) {
    const auto scaled = static_cast<std::int64_t>(logical) * normalized_dpi(dpi);
    const auto rounded = scaled < 0 ? -((-scaled + 48) / 96) : (scaled + 48) / 96;
    return rounded > std::numeric_limits<int>::max()
        ? std::numeric_limits<int>::max()
        : rounded < std::numeric_limits<int>::min()
        ? std::numeric_limits<int>::min()
        : static_cast<int>(rounded);
}
struct DpiBox { int x, y, width, height; };
inline constexpr DpiBox scaled_box(DpiBox b, unsigned dpi) {
    return {dpi_pixels(b.x,dpi), dpi_pixels(b.y,dpi),
            dpi_pixels(b.width,dpi), dpi_pixels(b.height,dpi)};
}
// Calculates the opaque bottom feedback panel and its text bounds in physical
// pixels. Clamp insets after the actual client size is known: a short or narrow
// window must never push explanatory text outside its own footer.
struct FooterBoxes {
    DpiBox panel;
    DpiBox message;
};
inline constexpr FooterBoxes footer_boxes(int client_width, int client_height,
                                           int preferred_height_pixels,
                                           unsigned dpi) {
    const int width = client_width > 0 ? client_width : 1;
    const int height = client_height > 0 ? client_height : 1;
    const int preferred = preferred_height_pixels > 0 ? preferred_height_pixels : 1;
    const int panel_height = preferred < height ? preferred : height;
    const int top = height - panel_height;
    const int desired_x = dpi_pixels(12, dpi);
    const int desired_y = dpi_pixels(5, dpi);
    const int inset_x = desired_x < (width - 1) / 2 ? desired_x : (width - 1) / 2;
    const int inset_y = desired_y < (panel_height - 1) / 2 ? desired_y : (panel_height - 1) / 2;
    return {{0, top, width, panel_height},
            {inset_x, top + inset_y, width - 2 * inset_x, panel_height - 2 * inset_y}};
}
} // namespace axv
