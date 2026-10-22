#include "../src/audio_hotplug_policy.h"
#include <windows.h>
#include <dbt.h>
#include <cstdint>
#include <limits>
int main() {
    using namespace axv;
    static_assert(DBT_DEVNODES_CHANGED == 7u, "Win32 device notification drift");
    static_assert(DBT_DEVICEARRIVAL == 0x8000u, "Win32 device notification drift");
    static_assert(DBT_DEVICEREMOVECOMPLETE == 0x8004u, "Win32 device notification drift");
    AudioHotplugPolicy p;
    if (p.notify(0x8001u, 1000) || p.pending()) return 1;
    if (!p.notify(DBT_DEVNODES_CHANGED, 1000) || !p.pending()) return 2;
    if (p.take_if_due(2000, false) || !p.pending()) return 3; // inactive tab
    if (p.take_if_due(1449, true) || !p.pending()) return 4;
    if (!p.notify(DBT_DEVICEARRIVAL, 1400)) return 5; // debounce storm
    if (p.take_if_due(1800, true) || !p.pending()) return 6;
    if (!p.take_if_due(1850, true) || p.pending()) return 7;
    if (p.take_if_due(3000, true)) return 8; // at most once
    if (!p.notify(DBT_DEVICEREMOVECOMPLETE, 3000)) return 9;
    p.acknowledge_manual_refresh();
    if (p.pending() || p.take_if_due(4000, true)) return 10;
    if (!p.notify(DBT_DEVNODES_CHANGED, 5000)) return 11;
    if (p.take_if_due(1, true) || !p.pending()) return 12; // non-monotonic input fails closed
    if (!p.take_if_due(5450, true)) return 13;
    auto near_max = (std::numeric_limits<std::uint64_t>::max)() - 500;
    if (!p.notify(DBT_DEVNODES_CHANGED, near_max)) return 14;
    if (p.take_if_due(near_max + 449, true)) return 15;
    if (!p.take_if_due(near_max + 450, true)) return 16;
    return 0;
}
