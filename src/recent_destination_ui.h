#pragma once
#include <windows.h>
#include "recent_destinations.h"
namespace axv {
// Deliberately has no engine parameter: choosing a recent item never dials.
inline bool fill_recent_destination(RecentDestinations& recents,std::uint64_t token,
                                    const SipRuntimeObservation& observed,HWND field) noexcept {
    recents.synchronize(observed);
    const auto* entry=recents.find(token,observed);
    if(!entry || !field)return false;
    try {
        // WM_SETTEXT may synchronously reenter UI callbacks that clear recents.
        const std::wstring destination=entry->destination;
        return SetWindowTextW(field,destination.c_str())!=0;
    }catch(...){return false;}
}
}
