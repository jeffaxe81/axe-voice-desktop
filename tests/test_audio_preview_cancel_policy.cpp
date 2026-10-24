#include "../src/audio_preview_cancel_policy.h"
int main() {
    using axv::AudioPreviewRole;
    using axv::preview_can_cancel;
    int allowed = 0;
    for (int running = 0; running != 2; ++running) {
        for (int already_cancelled = 0; already_cancelled != 2; ++already_cancelled) {
            for (int smoke = 0; smoke != 2; ++smoke) {
                for (int active = 0; active != 2; ++active) {
                    for (int clicked = 0; clicked != 2; ++clicked) {
                        const auto active_role = active ? AudioPreviewRole::Voice :
                            AudioPreviewRole::Ringtone;
                        const auto pressed_role = clicked ? AudioPreviewRole::Voice :
                            AudioPreviewRole::Ringtone;
                        const bool expected = running && !already_cancelled &&
                            !smoke && active == clicked;
                        const bool value = preview_can_cancel(running != 0,
                            already_cancelled != 0, smoke != 0, active_role, pressed_role);
                        if (expected != value) return 1;
                        if (value) ++allowed;
                    }
                }
            }
        }
    }
    if (allowed != 2) return 2;
    return 0;
}
