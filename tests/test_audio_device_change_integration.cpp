#include "../src/audio_device_change_policy.h"
#include "../src/audio_device_change_signal.h"
#include "../src/audio_inventory_async.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); std::abort(); } } while (false)

namespace {
// Gates control probe completion; policy time is entirely synthetic.
struct ProbeGate {
    std::mutex mutex;
    std::condition_variable cv;
    unsigned calls = 0;
    unsigned released = 0;

    axv::AudioInventory probe() {
        std::unique_lock<std::mutex> lock(mutex);
        const unsigned call = ++calls;
        cv.notify_all();
        CHECK(cv.wait_for(lock, std::chrono::seconds(5), [&] { return released >= call; }));
        axv::AudioInventory result;
        result.input_count = call;
        return result;
    }
    void entered(unsigned expected) {
        std::unique_lock<std::mutex> lock(mutex);
        CHECK(cv.wait_for(lock, std::chrono::seconds(5), [&] { return calls >= expected; }));
        CHECK(calls == expected);
    }
    void release(unsigned through) {
        std::lock_guard<std::mutex> lock(mutex);
        released = through;
        cv.notify_all();
    }
    unsigned count() {
        std::lock_guard<std::mutex> lock(mutex);
        return calls;
    }
};

void completed(axv::AudioInventoryAsync& worker) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (worker.snapshot().busy) {
        CHECK(std::chrono::steady_clock::now() < deadline);
        std::this_thread::yield();
    }
}

void notification_during_probe() {
    auto gate = std::make_shared<ProbeGate>();
    axv::AudioInventoryAsync worker([gate] { return gate->probe(); });
    axv::AudioDeviceChangePolicy policy;
    axv::AudioDeviceChangeSignal signal;
    axv::AudioInventory result;
    policy.enter(0);
    CHECK(worker.request());
    policy.refreshed(0);
    CHECK(worker.start_pending());
    gate->entered(1);

    // A storm coalesces into one consumed event and invalidates the active probe.
    for (unsigned i = 0; i < 1000; ++i) signal.notify();
    CHECK(signal.take());
    worker.cancel();
    policy.changed(100);
    CHECK(!signal.take());
    CHECK(!worker.take(result));
    CHECK(!worker.snapshot().pending);
    CHECK(!policy.should_refresh(349, false));
    CHECK(!policy.should_refresh(350, true));
    CHECK(!policy.should_refresh(800, true));
    CHECK(!worker.start_pending());
    CHECK(gate->count() == 1);

    CHECK(policy.should_refresh(800, false));
    CHECK(worker.request());
    policy.refreshed(800);
    CHECK(worker.snapshot().pending);
    CHECK(!policy.should_refresh(800, false));
    for (unsigned i = 0; i < 1000; ++i) CHECK(!worker.start_pending());
    CHECK(gate->count() == 1);
    CHECK(!worker.take(result));
    gate->release(1);
    completed(worker);
    CHECK(!worker.take(result));
    CHECK(worker.snapshot().pending);
    CHECK(worker.start_pending());
    gate->entered(2);
    CHECK(!worker.start_pending());
    gate->release(2);
    completed(worker);
    CHECK(worker.take(result));
    CHECK(result.input_count == 2);
    CHECK(!worker.take(result));
    CHECK(gate->count() == 2);
}

void fallback_leave_and_reenter() {
    auto gate = std::make_shared<ProbeGate>();
    axv::AudioInventoryAsync worker([gate] { return gate->probe(); });
    axv::AudioDeviceChangePolicy policy;
    axv::AudioInventory result;
    // No watcher/signal exists in this scenario: the periodic deadline suffices.
    policy.enter(1000);
    CHECK(!policy.should_refresh(5999, false));
    CHECK(!policy.should_refresh(6000, true));
    CHECK(policy.should_refresh(6000, false));
    CHECK(worker.request());
    policy.refreshed(6000);
    CHECK(!policy.should_refresh(6000, false));
    CHECK(worker.start_pending());
    gate->entered(1);
    policy.leave();
    worker.cancel();
    CHECK(!policy.should_refresh(20000, false));
    gate->release(1);
    completed(worker);
    CHECK(!worker.take(result));
    CHECK(!worker.start_pending());

    policy.enter(21000);
    CHECK(worker.request());
    policy.refreshed(21000);
    CHECK(worker.start_pending());
    gate->entered(2);
    gate->release(2);
    completed(worker);
    CHECK(worker.take(result));
    CHECK(result.input_count == 2);
    CHECK(!worker.take(result));
    // Leaving also discards an already completed result before any UI takes it.
    CHECK(worker.request());
    CHECK(worker.start_pending());
    gate->entered(3);
    gate->release(3);
    completed(worker);
    CHECK(worker.snapshot().ready);
    policy.leave();
    worker.cancel();
    CHECK(!worker.take(result));
}
void slow_probe_keeps_result_and_paces_from_completion() {
    auto gate=std::make_shared<ProbeGate>();
    axv::AudioInventoryAsync worker([gate]{return gate->probe();});
    axv::AudioDeviceChangePolicy policy;policy.enter(0);
    CHECK(worker.request());policy.refreshed(0);CHECK(worker.start_pending());gate->entered(1);
    const auto generation=worker.snapshot().generation;
    CHECK(policy.should_refresh(6000,false));
    CHECK(!axv::audio_device_refresh_due(policy,6000,false,worker.snapshot()));
    CHECK(worker.snapshot().generation==generation);
    gate->release(1);completed(worker);
    CHECK(!axv::audio_device_refresh_due(policy,7000,false,worker.snapshot()));
    axv::AudioInventory result;CHECK(worker.take(result));CHECK(result.input_count==1);
    policy.completed(7000);
    CHECK(!axv::audio_device_refresh_due(policy,11999,false,worker.snapshot()));
    CHECK(axv::audio_device_refresh_due(policy,12000,false,worker.snapshot()));
    // Completion updates periodic pacing without losing a later notification.
    policy.changed(12100);policy.completed(12110);
    CHECK(!axv::audio_device_refresh_due(policy,12349,false,worker.snapshot()));
    CHECK(axv::audio_device_refresh_due(policy,12350,false,worker.snapshot()));
}
} // namespace

int main() {
    notification_during_probe();
    fallback_leave_and_reenter();
    slow_probe_keeps_result_and_paces_from_completion();
}
