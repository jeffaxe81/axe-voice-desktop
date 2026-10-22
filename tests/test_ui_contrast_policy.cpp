#include "../src/ui_contrast_policy.h"
#include <cassert>

int main() {
    using namespace axv;
    constexpr UiSystemColors os{11,22,33,44,55,66,77};
    constexpr UiRole roles[] = {
        UiRole::Background, UiRole::Panel, UiRole::Field, UiRole::InvalidField,
        UiRole::Text, UiRole::Muted, UiRole::Accent, UiRole::Border,
        UiRole::Button, UiRole::Pressed, UiRole::DisabledButton,
        UiRole::SelectedText, UiRole::DisabledText, UiRole::ButtonText
    };
    constexpr UiColor expected[] = {
        11,11,11,11,22,22,55,22,33,55,33,66,77,44
    };
    static_assert(sizeof(roles)/sizeof(roles[0]) ==
                  sizeof(expected)/sizeof(expected[0]), "mismatched test");
    for (unsigned i = 0; i < sizeof(roles)/sizeof(roles[0]); ++i) {
        if (theme_color(roles[i], false, true, os) != expected[i]) return 1;
        if (theme_color(roles[i], true, true, os) != expected[i]) return 2;
    }
    if (theme_color(UiRole::Background, true, false, os) != rgb(14,22,38)) return 3;
    if (theme_color(UiRole::Background, false, false, os) != rgb(245,248,252)) return 4;
    if (theme_color(UiRole::InvalidField, false, false, os) != rgb(255,231,235)) return 5;
    if (theme_color(UiRole::Accent, true, false, os) != rgb(38,178,198)) return 6;
    if (theme_color(UiRole::SelectedText, true, false, os) != rgb(5,25,38)) return 7;
    return 0;
}
