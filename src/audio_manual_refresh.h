#pragma once

namespace axv {

// Pure, UI-thread-owned policy. A user-triggered refresh arriving while a
// Core Audio scan is busy must be retried once after that worker goes idle.
// No endpoint IDs, account data or Baresip state are stored here.
class AudioManualRefresh {
public:
    void defer() { pending_ = true; }
    void started() { pending_ = false; }
    void cancel() { pending_ = false; }

    bool pending() const { return pending_; }
    bool retry_ready(bool worker_busy) const { return pending_ && !worker_busy; }

private:
    bool pending_ = false;
};

} // namespace axv
