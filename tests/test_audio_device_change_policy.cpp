#include "../src/audio_device_change_policy.h"

int main() {
    using axv::should_schedule_audio_device_refresh;
    const unsigned events[] = {
        DBT_DEVNODES_CHANGED, DBT_DEVICEARRIVAL,
        DBT_DEVICEREMOVECOMPLETE, DBT_CONFIGCHANGED
    };
    for (unsigned e : events) {
        if (!should_schedule_audio_device_refresh(e, true, false)) return 1;
        if (should_schedule_audio_device_refresh(e, false, false)) return 2;
        if (should_schedule_audio_device_refresh(e, true, true)) return 3;
    }
    if (should_schedule_audio_device_refresh(DBT_DEVICEQUERYREMOVE, true, false))
        return 4;
    if (should_schedule_audio_device_refresh(DBT_DEVICEREMOVEPENDING, true, false))
        return 5;
    if (should_schedule_audio_device_refresh(0, true, false)) return 6;
    // No SIP/media proof is derived from these events.
    return 0;
}
