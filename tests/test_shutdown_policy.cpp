#include "../src/shutdown_policy.h"
#include <cstdint>

int main() {
    axv::ShutdownDeadline deadline;
    if (deadline.pending() || deadline.expired(1500, 1500)) return 1;
    if (!deadline.begin(1000) || !deadline.pending()) return 2;
    if (deadline.begin(1400)) return 3;
    if (deadline.expired(2499, 1500)) return 4;
    if (!deadline.expired(2500, 1500)) return 5;
    deadline.clear();
    if (deadline.pending() || deadline.expired(10000, 1500)) return 6;
    if (!deadline.begin(20000) || deadline.expired(20001, 1500)) return 7;
    // Unsigned subtraction tolerates monotonic millisecond counter wraparound.
    deadline.clear();
    if (!deadline.begin(UINT64_MAX - 200)) return 8;
    if (!deadline.expired(1300, 1500)) return 9;
    return 0;
}
