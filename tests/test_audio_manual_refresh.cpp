#include "../src/audio_manual_refresh.h"
#include "../src/audio_inventory_async.h"
#include <chrono>
#include <future>
#include <thread>

int main() {
    using namespace std::chrono_literals;
    axv::AudioManualRefresh policy;
    if (policy.pending() || policy.retry_ready(false)) return 1;
    policy.defer(); // First user refresh during an in-flight scan.
    if (!policy.pending() || policy.retry_ready(true)) return 2;
    policy.defer(); // Repeated clicks coalesce to one pending request.
    if (!policy.retry_ready(false)) return 3;
    policy.started();
    if (policy.pending() || policy.retry_ready(false)) return 4;
    policy.defer();
    policy.cancel(); // Navigation or window shutdown.
    if (policy.pending() || policy.retry_ready(false)) return 5;

    // Exercise the policy with the real worker transport, using synthetic
    // inventory data and explicitly controlled scan completion.
    std::promise<void> started, release;
    auto entered = started.get_future();
    auto gate = release.get_future().share();
    int count = 0;
    axv::AudioInventoryAsync worker([&]() {
        const int n = ++count; // No simultaneous inspections are allowed.
        if (n == 1) {
            started.set_value();
            gate.wait();
        }
        axv::AudioInventory result;
        result.input_count = static_cast<unsigned int>(n);
        return result;
    });
    if (!worker.request()) return 6;
    if (entered.wait_for(2s) != std::future_status::ready) {
        release.set_value(); return 7;
    }
    worker.invalidate(); // Suppress stale result from first scan.
    policy.defer();
    if (!policy.pending() || policy.retry_ready(worker.busy())) {
        release.set_value(); return 8;
    }
    release.set_value();
    for (int i = 0; i < 200 && worker.busy(); ++i)
        std::this_thread::sleep_for(10ms);
    if (!policy.retry_ready(worker.busy())) return 9;
    if (!worker.request()) return 10;
    policy.started();
    for (int i = 0; i < 200 && worker.busy(); ++i)
        std::this_thread::sleep_for(10ms);
    const auto latest = worker.take();
    if (!latest || latest->input_count != 2 || worker.take().has_value())
        return 11;
    if (policy.pending() || count != 2) return 12;
    return 0;
}
