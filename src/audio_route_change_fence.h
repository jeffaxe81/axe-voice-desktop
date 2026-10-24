#pragma once
#include <string>
#include <utility>

namespace axv {

// UI-only routing intent. No endpoint, SIP or media operation occurs here.
enum class AudioRouteChangeKind { None, Refresh, Swap, Microphone, Voice, Ringtone };
struct AudioRouteChange {
    AudioRouteChangeKind kind = AudioRouteChangeKind::None;
    std::wstring endpoint_id;
};

// A route change requested during a preview is deferred until the old local
// sound thread has actually been joined. Latest user intent replaces older one.
// The caller revalidates the endpoint ID against its current inventory on apply.
class AudioRouteChangeFence {
public:
    bool defer_if_busy(const AudioRouteChange& change, bool preview_busy) {
        if (!preview_busy || change.kind == AudioRouteChangeKind::None) return false;
        pending_ = change;
        return true;
    }

    bool has_pending() const { return pending_.kind != AudioRouteChangeKind::None; }

    AudioRouteChange take_after_stop_ack(bool stopped_and_joined) {
        if (!stopped_and_joined) return {};
        AudioRouteChange taken = std::move(pending_);
        pending_ = {};
        return taken;
    }

    void clear() { pending_ = {}; }

private:
    AudioRouteChange pending_{};
};

} // namespace axv
