#include "audio_inventory_async.h"
#include <limits>
#include <mutex>
#include <thread>
#include <utility>

namespace axv {
struct AudioInventoryAsync::State {
    explicit State(Probe value) : probe(std::move(value)) {}
    std::mutex mutex;
    Probe probe;
    AudioInventoryAsyncSnapshot status;
    AudioInventory result;

    bool invalidate() {
        status.pending = false;
        status.ready = false;
        result = AudioInventory{};
        if (status.generation == std::numeric_limits<std::uint64_t>::max()) {
            status.closed = true;
            return false;
        }
        ++status.generation;
        return true;
    }
};

AudioInventoryAsync::AudioInventoryAsync(Probe probe)
    : state_(std::make_shared<State>(std::move(probe))) {}
AudioInventoryAsync::~AudioInventoryAsync() { close(); }

bool AudioInventoryAsync::request() {
    const auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->status.closed || !state->invalidate()) return false;
    state->status.pending = true;
    return true;
}

bool AudioInventoryAsync::start_pending() {
    const auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    auto& status = state->status;
    if (status.closed || status.busy || !status.pending) return false;
    const auto generation = status.generation;
    status.busy = true;
    // Construct while holding the lock, so completion cannot race publication.
    std::thread thread;
    try {
        thread = std::thread([state, generation] {
            AudioInventory result;
            try {
                result = state->probe();
            } catch (...) {
                result.error = AudioInventoryError::EndpointQueryFailed;
            }
            std::lock_guard<std::mutex> completed(state->mutex);
            state->status.busy = false;
            if (!state->status.closed && state->status.generation == generation) {
                state->result = std::move(result);
                state->status.ready = true;
            }
        });
    } catch (...) {
        status.busy = false;
        status.launch_failed = true;
        return false;
    }
    thread.detach();
    status.pending = false;
    status.launch_failed = false;
    return true;
}

bool AudioInventoryAsync::take(AudioInventory& inventory) {
    const auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->status.closed || !state->status.ready) return false;
    inventory = std::move(state->result);
    state->status.ready = false;
    return true;
}
void AudioInventoryAsync::cancel() {
    const auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (!state->status.closed) state->invalidate();
}
void AudioInventoryAsync::close() {
    const auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (!state->status.closed) state->invalidate();
    state->status.closed = true;
}
AudioInventoryAsyncSnapshot AudioInventoryAsync::snapshot() const {
    const auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->status;
}
} // namespace axv
