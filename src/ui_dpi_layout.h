#pragma once
#include <cstdint>
namespace axv {
struct UiLogicalRect { int x, y, width, height; };
inline int scale_dpi(int value, unsigned dpi) {
    const unsigned safe = dpi < 96 ? 96u : (dpi > 768 ? 768u : dpi);
    const std::int64_t n = static_cast<std::int64_t>(value) * safe;
    return static_cast<int>(n < 0 ? -((-n + 48) / 96) : (n + 48) / 96);
}
inline UiLogicalRect scale_rect(UiLogicalRect r, unsigned dpi) {
    return {scale_dpi(r.x, dpi), scale_dpi(r.y, dpi),
            scale_dpi(r.width, dpi), scale_dpi(r.height, dpi)};
}
} // namespace axv
