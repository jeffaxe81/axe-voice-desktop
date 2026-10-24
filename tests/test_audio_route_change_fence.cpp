#include "../src/audio_route_change_fence.h"
#include <string>

int main() {
    using namespace axv;
    AudioRouteChangeFence fence;
    const AudioRouteChange refresh{AudioRouteChangeKind::Refresh, {}};
    const AudioRouteChange swap{AudioRouteChangeKind::Swap, {}};
    const AudioRouteChange voice{AudioRouteChangeKind::Voice, L"headset-output"};
    const AudioRouteChange ring{AudioRouteChangeKind::Ringtone, L"pc-speaker"};
    if (fence.has_pending()) return 1;
    if (fence.defer_if_busy(swap, false) || fence.has_pending()) return 2;
    if (fence.defer_if_busy({}, true) || fence.has_pending()) return 3;
    if (!fence.defer_if_busy(refresh, true) || !fence.has_pending()) return 4;
    if (fence.take_after_stop_ack(false).kind != AudioRouteChangeKind::None ||
        !fence.has_pending()) return 5;
    if (!fence.defer_if_busy(swap, true)) return 6;
    if (!fence.defer_if_busy(voice, true)) return 7;
    if (!fence.defer_if_busy(ring, true)) return 8;
    const auto latest = fence.take_after_stop_ack(true);
    if (latest.kind != AudioRouteChangeKind::Ringtone ||
        latest.endpoint_id != L"pc-speaker" || fence.has_pending()) return 9;
    if (fence.take_after_stop_ack(true).kind != AudioRouteChangeKind::None) return 10;
    if (!fence.defer_if_busy(voice, true)) return 11;
    fence.clear();
    if (fence.has_pending() ||
        fence.take_after_stop_ack(true).kind != AudioRouteChangeKind::None) return 12;
    // Stress interleaved user input while the worker has not signalled STOP.
    for (int i = 0; i < 10000; ++i) {
        const AudioRouteChange a{i % 2 ? AudioRouteChangeKind::Voice :
            AudioRouteChangeKind::Ringtone, std::to_wstring(i)};
        if (!fence.defer_if_busy(a, true)) return 13;
        if (fence.take_after_stop_ack(false).kind != AudioRouteChangeKind::None) return 14;
        const auto acknowledged = fence.take_after_stop_ack(true);
        if (acknowledged.kind != a.kind || acknowledged.endpoint_id != a.endpoint_id) return 15;
        if (fence.has_pending()) return 16;
    }
    return 0;
}
