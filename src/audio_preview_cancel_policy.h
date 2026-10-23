#pragma once
#include "audio_preview_target.h"

namespace axv {
// A stop action belongs exclusively to the active local test;
// the other role cannot stop/start a second worker, and smoke never plays.
inline bool preview_can_cancel(bool running, bool cancellation_requested,
    bool smoke_mode, AudioPreviewRole active, AudioPreviewRole pressed) {
    return running && !cancellation_requested && !smoke_mode && active == pressed;
}
} // namespace axv
