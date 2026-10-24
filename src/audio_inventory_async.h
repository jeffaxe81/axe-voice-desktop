#pragma once
#include "audio_devices.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace axv {

// Core Audio enumeration runs off the Win32 UI thread. The worker never
// handles HWND, SIP credentials or audio streams. Detached workers own
// their shared state until completion, so UI shutdown cannot block on
// an unresponsive Windows audio driver.
class AudioInventoryAsync {
public:
    using Inspector = std::function<AudioInventory()>;
    explicit AudioInventoryAsync(Inspector inspector)
        : state_(std::make_shared<State>()), inspect_(std::move(inspector)) {}
    AudioInventoryAsync(const AudioInventoryAsync&) = delete;
    AudioInventoryAsync& operator=(const AudioInventoryAsync&) = delete;
    ~AudioInventoryAsync() { invalidate(); }

    bool request() {
        auto state = state_;
        std::uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->busy) return false;
            state->busy = true;
            generation = state->generation;
        }
        try {
            std::thread([state, inspector = inspect_, generation]() {
                AudioInventory result;
                try {
                    result = inspector(); // Inspector initializes its own COM.
                } catch (...) {
                    result.error = AudioInventoryError::EndpointQueryFailed;
                }
                std::lock_guard<std::mutex> lock(state->mutex);
                if (generation == state->generation)
                    state->ready = std::move(result);
                state->busy = false;
            }).detach();
        } catch (...) {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy = false;
            return false;
        }
        return true;
    }

    std::optional<AudioInventory> take() {
        std::lock_guard<std::mutex> lock(state_->mutex);
        auto result = std::move(state_->ready);
        state_->ready.reset();
        return result;
    }

    void invalidate() {
        std::lock_guard<std::mutex> lock(state_->mutex);
        ++state_->generation;
        state_->ready.reset();
    }

    bool busy() const {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->busy;
    }
private:
    struct State {
        std::mutex mutex;
        bool busy = false;
        std::uint64_t generation = 0;
        std::optional<AudioInventory> ready;
    };
    std::shared_ptr<State> state_;
    Inspector inspect_;
};
} // namespace axv
