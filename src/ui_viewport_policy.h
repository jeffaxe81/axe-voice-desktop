#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace axv {
// Use 64-bit intermediates even for transient/invalid HWND dimensions.
// Overflow here can send an off-screen control far outside its viewport.
inline int max_scroll_offset(int content_height, int visible_height) {
    const auto diff = static_cast<std::int64_t>(content_height) -
                      static_cast<std::int64_t>(std::max(0, visible_height));
    return static_cast<int>(std::max<std::int64_t>(0,
        std::min<std::int64_t>(diff, std::numeric_limits<int>::max())));
}
inline int clamp_scroll_offset(int requested, int content_height, int visible_height) {
    return std::max(0, std::min(max_scroll_offset(content_height, visible_height), requested));
}
inline int scroll_for_visible_range(int current, int content, int visible,
                                    int top, int bottom, int margin=0) {
    const auto inset = static_cast<std::int64_t>(std::max(0, margin));
    const auto page = static_cast<std::int64_t>(std::max(0, visible));
    // Bounds may be inconsistent during live resizing; never use int
    // subtraction/addition before promoting to a wider type.
    std::int64_t next = current;
    if (static_cast<std::int64_t>(top) - inset < current) {
        next = static_cast<std::int64_t>(top) - inset;
    } else if (static_cast<std::int64_t>(bottom) + inset >
               static_cast<std::int64_t>(current) + page) {
        next = (static_cast<std::int64_t>(bottom) - top + 2*inset > page)
            ? static_cast<std::int64_t>(top) - inset
            : static_cast<std::int64_t>(bottom) + inset - page;
    }
    // Apply saturation after computing the target; the scroll range is nonnegative.
    return static_cast<int>(std::max<std::int64_t>(0,
        std::min<std::int64_t>(next, max_scroll_offset(content, visible))));
}
} // namespace axv
