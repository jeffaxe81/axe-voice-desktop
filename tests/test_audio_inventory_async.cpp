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

    // A completed but unconsumed result must not leak into the next scan.
    std::promise<void> rescan_started, rescan_release;
    const auto rescan_started_future = rescan_started.get_future();
    auto rescan_gate = rescan_release.get_future().share();
    std::atomic<int> sequence{0};
    axv::AudioInventoryAsync rescans([&]() {
        const int n = ++sequence;
        if (n == 2) {
            rescan_started.set_value();
            rescan_gate.wait();
        }
        axv::AudioInventory x;
        x.input_count = static_cast<unsigned int>(n);
        return x;
    });
    if (!rescans.request() || !finish_within(rescans)) return 13;
    if (!rescans.request()) return 14;
    if (rescan_started_future.wait_for(2s) != std::future_status::ready) {
        rescan_release.set_value(); return 15;
    }
    if (rescans.take().has_value()) {
        rescan_release.set_value(); return 16;
    }
    rescan_release.set_value();
    if (!finish_within(rescans)) return 17;
    const auto latest = rescans.take();
    if (!latest || latest->input_count != 2 || rescans.take().has_value())
        return 18;
    return 0;
}
