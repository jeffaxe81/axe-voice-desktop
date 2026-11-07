#pragma once
// DPI-aware Win32 rounded geometry shared by cards and owner-drawn buttons.
// RoundRect's corner arguments are FULL ellipse diameters, unlike CSS radii.
#include "ui_dpi_policy.h"
#include <algorithm>
#include <limits>

namespace axv {
inline constexpr int rounded_rect_diameter(int width_px, int height_px,
                                           int radius_px, int inset_px=0) noexcept {
    if(width_px<=0 || height_px<=0 || radius_px<=0) return 0;
    const int inset = inset_px>0 ? inset_px : 0;
    // Avoid overflow in width - 2*inset and radius*2 for corrupted dimensions.
    if(inset > (width_px-1)/2 || inset > (height_px-1)/2) return 0;
    const int safe_width = width_px - 2*inset;
    const int safe_height = height_px - 2*inset;
    const int max_diameter = (std::min)(safe_width,safe_height);
    const int requested = radius_px > (std::numeric_limits<int>::max)()/2
        ? (std::numeric_limits<int>::max)() : 2*radius_px;
    return (std::min)(requested,max_diameter);
}

inline constexpr int compact_button_diameter(int width_px,int height_px,unsigned dpi,
                                             bool essential_mode,int inset_px) noexcept {
    return rounded_rect_diameter(width_px,height_px,
                                 dpi_pixels(essential_mode?20:14,dpi),inset_px);
}
} // namespace axv
