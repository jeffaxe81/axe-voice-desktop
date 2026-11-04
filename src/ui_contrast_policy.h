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
    case UiRole::Background: return dark ? rgb(20,24,32) : rgb(248,250,252);
    case UiRole::Panel: return dark ? rgb(31,37,48) : rgb(255,255,255);
    case UiRole::Field: return dark ? rgb(43,51,65) : rgb(241,245,249);
    case UiRole::InvalidField: return dark ? rgb(78,41,49) : rgb(254,226,226);
    case UiRole::Text: return dark ? rgb(236,240,246) : rgb(30,41,59);
    case UiRole::Muted: return dark ? rgb(163,174,191) : rgb(100,116,139);
    case UiRole::Accent: return dark ? rgb(60,174,198) : rgb(30,116,149);
    case UiRole::Border: return dark ? rgb(65,76,92) : rgb(226,232,240);
    case UiRole::Button: return dark ? rgb(39,47,60) : rgb(255,255,255);
    case UiRole::Pressed: return dark ? rgb(41,99,120) : rgb(215,237,244);
    case UiRole::DisabledButton: return dark ? rgb(56,63,76) : rgb(226,232,240);
    case UiRole::SelectedText: return rgb(255,255,255);
    case UiRole::DisabledText: return dark ? rgb(163,174,191) : rgb(100,116,139);
    case UiRole::ButtonText: return dark ? rgb(236,240,246) : rgb(30,41,59);
    }
    return os.window_text;
}
} // namespace axv
