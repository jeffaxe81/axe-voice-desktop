#pragma once
#include <cstdint>

namespace axv {
// COLORREF-compatible 0x00bbggrr values; no Win32 dependency in unit tests.
using UiColor = std::uint32_t;
constexpr UiColor rgb(unsigned r, unsigned g, unsigned b) {
    return (b << 16) | (g << 8) | r;
}
struct UiSystemColors {
    UiColor window = 0;
    UiColor window_text = 0;
    UiColor button_face = 0;
    UiColor button_text = 0;
    UiColor highlight = 0;
    UiColor highlight_text = 0;
    UiColor gray_text = 0;
};
enum class UiRole {
    Background, Panel, Field, InvalidField, Text, Muted, Accent, Border,
    Button, Pressed, DisabledButton, SelectedText, DisabledText, ButtonText
};

// A user-selected dark/light theme MUST NOT override Windows High Contrast.
constexpr UiColor theme_color(UiRole role, bool dark, bool high_contrast,
                               const UiSystemColors& os) {
    if (high_contrast) {
        switch (role) {
        case UiRole::Background: case UiRole::Panel:
        case UiRole::Field: case UiRole::InvalidField: return os.window;
        case UiRole::Text: case UiRole::Muted: case UiRole::Border: return os.window_text;
        case UiRole::Accent: case UiRole::Pressed: return os.highlight;
        case UiRole::Button: case UiRole::DisabledButton: return os.button_face;
        case UiRole::SelectedText: return os.highlight_text;
        case UiRole::DisabledText: return os.gray_text;
        case UiRole::ButtonText: return os.button_text;
        }
    }
    switch (role) {
    case UiRole::Background: return dark ? rgb(9,24,49) : rgb(246,249,253);
    case UiRole::Panel: return dark ? rgb(16,40,75) : rgb(255,255,255);
    case UiRole::Field: return dark ? rgb(27,57,93) : rgb(235,243,251);
    case UiRole::InvalidField: return dark ? rgb(79,39,53) : rgb(255,231,235);
    case UiRole::Text: return dark ? rgb(241,248,255) : rgb(17,43,78);
    case UiRole::Muted: return dark ? rgb(175,199,225) : rgb(90,113,141);
    case UiRole::Accent: return rgb(0,170,238);
    case UiRole::Border: return dark ? rgb(45,83,125) : rgb(207,223,239);
    case UiRole::Button: return dark ? rgb(25,38,58) : rgb(255,255,255);
    case UiRole::Pressed: return dark ? rgb(28,92,145) : rgb(210,233,250);
    case UiRole::DisabledButton: return dark ? rgb(42,53,70) : rgb(222,228,235);
    case UiRole::SelectedText: return rgb(4,30,56);
    case UiRole::DisabledText: return dark ? rgb(157,176,200) : rgb(91,108,128);
    case UiRole::ButtonText: return dark ? rgb(236,244,255) : rgb(20,38,62);
    }
    return os.window_text;
}
} // namespace axv
