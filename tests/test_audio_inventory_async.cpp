#include "../src/audio_inventory_async.h"
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

namespace {
bool finish_within(axv::AudioInventoryAsync& worker) {
    using namespace std::chrono_literals;
    for (int i = 0; i != 200; ++i) {
        if (!worker.busy()) return true;
        std::this_thread::sleep_for(10ms);
    }
    return false;
}
}
int main() {
    using namespace std::chrono_literals;
    std::promise<void> started, release;
    auto started_future = started.get_future();
    auto gate = release.get_future().share();
    std::atomic<int> calls{0};
    axv::AudioInventoryAsync worker([&]() {
        const int n = ++calls;
        if (n == 1) {
            started.set_value();
            gate.wait();
        }
        axv::AudioInventory inventory;
        inventory.input_count = static_cast<unsigned int>(n);
        return inventory;
    });
    if (!worker.request()) return 1;
    if (started_future.wait_for(2s) != std::future_status::ready) {
        release.set_value(); return 2;
    }
    if (!worker.busy() || worker.take().has_value()) {
        release.set_value(); return 3;
    }
    if (worker.request()) { release.set_value(); return 4; }
    worker.invalidate();  // Navigating away discards late callbacks.
    release.set_value();
    if (!finish_within(worker) || worker.take().has_value()) return 5;
    if (!worker.request() || !finish_within(worker)) return 6;
    auto result = worker.take();
    if (!result || result->input_count != 2 || worker.take().has_value()) return 7;
    if (calls.load() != 2) return 8;

    axv::AudioInventoryAsync errors([]() -> axv::AudioInventory { throw 5; });
    if (!errors.request() || !finish_within(errors)) return 9;
    auto failure = errors.take();
    if (!failure || failure->error != axv::AudioInventoryError::EndpointQueryFailed)
        return 10;

    std::promise<void> slow_start, slow_release;
    auto slow_started = slow_start.get_future();
    auto slow_gate = slow_release.get_future().share();
    {
        axv::AudioInventoryAsync closing([&]() {
            slow_start.set_value();
            slow_gate.wait();
            return axv::AudioInventory{};
        });
        if (!closing.request()) return 11;
        if (slow_started.wait_for(2s) != std::future_status::ready) {
            slow_release.set_value(); return 12;
        }
    }
    slow_release.set_value();
    return 0;
}
