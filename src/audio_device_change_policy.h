#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbt.h>

namespace axv {

// The OS message only requests a new local catalog read: it does not prove
// availability, permissions, Baresip media or SIP registration.
// Some changes (e.g. a new default communications endpoint) will not emit
// WM_DEVICECHANGE; a periodic Core Audio check remains necessary.
inline bool should_schedule_audio_device_refresh(unsigned int event,
                                                 bool audio_page_visible,
                                                 bool smoke_mode) {
    return !smoke_mode && audio_page_visible &&
        (event == DBT_DEVNODES_CHANGED ||
         event == DBT_DEVICEARRIVAL ||
         event == DBT_DEVICEREMOVECOMPLETE ||
         event == DBT_CONFIGCHANGED);
}
} // namespace axv
