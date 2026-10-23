#include "../src/audio_inventory_async.h"
#include "../src/audio_manual_refresh.h"
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

namespace {
bool finish(axv::AudioInventoryAsync& worker) {
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
    const auto gate = release.get_future().share();
    std::atomic<unsigned> queries{0};

    axv::AudioInventoryAsync settings([&]() {
        const unsigned n = ++queries;
        if (n == 1) {
            started.set_value();
            gate.wait();
        }
        axv::AudioInventory snapshot;
        snapshot.input_count = n;
        return snapshot;
    });
    axv::AudioManualRefresh pending;

    if (!settings.request()) return 1;
    if (started_future.wait_for(2s) != std::future_status::ready) {
        release.set_value(); return 2;
    }
    // User leaves Settings. A late result must never become visible.
    settings.invalidate();
    pending.cancel();
    if (settings.take().has_value() || pending.pending()) {
        release.set_value(); return 3;
    }
    // The user returns while the previous driver request is still blocked.
    if (!settings.busy()) {
        release.set_value(); return 4;
    }
    settings.invalidate();
    pending.defer();
    pending.defer(); // Repeated clicks collapse into one fresh scan.
    if (pending.retry_ready(settings.busy())) {
        release.set_value(); return 5;
    }
    release.set_value();
    if (!finish(settings)) return 6;
    if (settings.take().has_value() || !pending.retry_ready(settings.busy()))
        return 7;

    if (!settings.request()) return 8;
    pending.started();
    if (!finish(settings)) return 9;
    auto latest = settings.take();
    if (!latest || latest->input_count != 2 || settings.take().has_value())
        return 10;
    if (queries.load() != 2 || pending.pending()) return 11;

    // A second navigation cancels pending refresh without needing to join
    // an unresponsive driver thread or interact with a real headset.
    pending.defer();
    settings.invalidate();
    pending.cancel();
    if (pending.retry_ready(false) || settings.take().has_value()) return 12;

    // Settings and Audio use independent workers and do not share results.
    axv::AudioInventoryAsync audio([]() {
        axv::AudioInventory x;
        x.output_count = 3;
        return x;
    });
    if (!audio.request() || !finish(audio)) return 13;
    const auto audio_result = audio.take();
    if (!audio_result || audio_result->output_count != 3 ||
        settings.take().has_value()) return 14;

    return 0;
}
