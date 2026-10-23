#include "../src/audio_hotplug_debounce.h"
int main() {
    axv::AudioHotplugDebounce s;
    if (s.pending() || s.consume_if_due(1,true)) return 1;
    s.notify(1000);
    if (!s.pending() || s.consume_if_due(1399,true)) return 2;
    if (s.consume_if_due(1400,false) || !s.pending()) return 3;
    if (!s.consume_if_due(1400,true) || s.pending()) return 4;
    s.notify(5000); s.notify(5150); s.notify(5225);
    if (s.consume_if_due(5400,true)) return 5;
    if (!s.consume_if_due(5625,true)) return 6;
    s.notify(7000);
    if (s.consume_if_due(9000,false) || !s.pending()) return 7;
    s.clear();
    if (s.pending() || s.consume_if_due(9000,true)) return 8;
    s.notify(15000);
    if (s.consume_if_due(14999,true)) return 9;
    if (!s.consume_if_due(15400,true)) return 10;
    return 0;
}
