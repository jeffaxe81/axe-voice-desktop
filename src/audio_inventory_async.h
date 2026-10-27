#pragma once
#include "audio_devices.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace axv {
struct AudioInventoryAsyncSnapshot {
    std::uint64_t generation = 0;
    bool busy = false;
    bool pending = false;
    bool ready = false;
    bool closed = false;
    bool launch_failed = false;
};

// Probe must own its dependencies: a detached driver call can outlive this object.
class AudioInventoryAsync {
public:
    using Probe = std::function<AudioInventory()>;
    explicit AudioInventoryAsync(Probe probe);
    ~AudioInventoryAsync();
    AudioInventoryAsync(const AudioInventoryAsync&) = delete;
    AudioInventoryAsync& operator=(const AudioInventoryAsync&) = delete;
    bool request();
    bool start_pending();
    bool take(AudioInventory& inventory);
    void cancel();
    void close();
    AudioInventoryAsyncSnapshot snapshot() const;
private:
    struct State;
    std::shared_ptr<State> state_;
};
} // namespace axv
