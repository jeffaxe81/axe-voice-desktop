#include "../src/audio_inventory_retry.h"
#include <cstdint>
#include <limits>

int main() {
    axv::AudioInventoryRetry retry;
    if (!retry.ready(0)) return 1;
    retry.failed(0);
    if (retry.ready(0) || retry.ready(999) || !retry.ready(1000)) return 2;
    retry.failed(1000);
    if (retry.ready(1999) || !retry.ready(2000)) return 3;
    retry.failed(1500);
    if (retry.ready(2499) || !retry.ready(2500)) return 4;
    // A backward timestamp must never shorten an existing deadline.
    retry.failed(100);
    if (retry.ready(1100) || retry.ready(2499) || !retry.ready(2500)) return 5;
    retry.reset();
    if (!retry.ready(0)) return 6;
    retry.failed(100);
    retry.started();
    if (!retry.ready(0)) return 7;
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    retry.failed(maximum - 500);
    if (retry.ready(0) || retry.ready(maximum - 1) || !retry.ready(maximum)) return 8;
    retry.reset();
    retry.failed(maximum - 1000);
    if (retry.ready(maximum - 1) || !retry.ready(maximum)) return 9;
    retry.reset();
    retry.failed(maximum);
    if (retry.ready(maximum - 1) || !retry.ready(maximum)) return 10;
    retry.started();
    if (!retry.ready(0) || !retry.ready(maximum)) return 11;
    return 0;
}
