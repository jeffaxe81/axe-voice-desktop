#include "../src/audio_preview_levels.h"
#include <cstdint>
#include <cmath>
int main() {
    using namespace axv;
    if (kPreviewDefaultLevel != 75 || kPreviewLevels.size() != 4) return 1;
    if (preview_level_index(25) != 0 || preview_level_index(50) != 1 ||
        preview_level_index(75) != 2 || preview_level_index(100) != 3 ||
        preview_level_index(13) != -1) return 2;
    if (preview_level_for_index(0, 75) != 25 ||
        preview_level_for_index(1, 75) != 50 ||
        preview_level_for_index(2, 75) != 75 ||
        preview_level_for_index(3, 75) != 100 ||
        preview_level_for_index(-1, 75) != 75 ||
        preview_level_for_index(4, 50) != 50) return 3;
    if (preview_level_valid(101) || !preview_level_valid(0) ||
        !preview_level_valid(100)) return 4;
    if (std::fabs(preview_scale(.11f, 25) - .0275f) > 0.000001f ||
        std::fabs(preview_scale(-.11f, 50) + .055f) > 0.000001f ||
        std::fabs(preview_scale(.11f, 75) - .0825f) > 0.000001f ||
        std::fabs(preview_scale(.11f, 100) - .11f) > 0.000001f ||
        preview_scale(.11f, 0) != 0.f ||
        preview_scale(.11f, 101) != 0.f) return 5;
    // Each role keeps its own preference, independent of a device swap.
    std::uint8_t voice = kPreviewDefaultLevel;
    std::uint8_t ring = kPreviewDefaultLevel;
    voice = preview_level_for_index(0, voice);
    ring = preview_level_for_index(3, ring);
    if (voice != 25 || ring != 100) return 6;
    // UI role swap exchanges only endpoint IDs, never local gain preferences.
    voice = preview_level_for_index(-1, voice);
    ring = preview_level_for_index(4, ring);
    if (voice != 25 || ring != 100) return 7;
    return 0;
}
