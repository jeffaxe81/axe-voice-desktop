#include "../src/audio_inventory_async.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
    std::abort(); \
} } while (false)

namespace {
constexpr auto timeout = std::chrono::seconds(3);

struct Gate {
    explicit Gate(std::shared_ptr<std::promise<void>> destroyed_signal)
        : destroyed(std::move(destroyed_signal)) {}
    ~Gate() { destroyed->set_value(); }

    axv::AudioInventory probe() {
        std::unique_lock<std::mutex> lock(mutex);
        entered = true;
        changed.notify_all();
        // A broken test must fail instead of leaving a detached thread forever.
        CHECK(changed.wait_for(lock, timeout, [this] { return released; }));
        returned = true;
        changed.notify_all();
        axv::AudioInventory inventory;
        inventory.input_count = 73;
        return inventory;
    }

    void wait_entered() {
        std::unique_lock<std::mutex> lock(mutex);
        CHECK(changed.wait_for(lock, timeout, [this] { return entered; }));
    }
    void release() {
        std::lock_guard<std::mutex> lock(mutex);
        CHECK(entered && !returned);
        released = true;
        changed.notify_all();
    }
    void check_still_blocked() {
        std::lock_guard<std::mutex> lock(mutex);
        CHECK(entered && !released && !returned);
    }

    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool released = false;
    bool returned = false;
    std::shared_ptr<std::promise<void>> destroyed;
};

void check_closed(axv::AudioInventoryAsync& worker) {
    const auto status = worker.snapshot();
    CHECK(status.closed && !status.pending && !status.ready);
    CHECK(!worker.request());
    CHECK(!worker.start_pending());
    axv::AudioInventory untouched;
    untouched.input_count = 91;
    CHECK(!worker.take(untouched));
    CHECK(untouched.input_count == 91);
    worker.close();
    worker.cancel();
    CHECK(worker.snapshot().generation == status.generation);
    CHECK(worker.snapshot().closed);
}

void close_while_probe_is_blocked() {
    auto destroyed = std::make_shared<std::promise<void>>();
    auto reclaimed = destroyed->get_future();
    auto gate = std::make_shared<Gate>(destroyed);
    const std::weak_ptr<Gate> weak = gate;
    auto worker = std::make_unique<axv::AudioInventoryAsync>([gate] { return gate->probe(); });
    CHECK(worker->request() && worker->start_pending());
    gate->wait_entered();
    // Exercise invalidation of a queued follow-up as well as the active probe.
    CHECK(worker->request());
    CHECK(worker->snapshot().pending);

    std::promise<void> closed;
    auto close_returned = closed.get_future();
    std::thread closer([&] { worker->close(); closed.set_value(); });
    // The gate is released only after close has returned: no driver time limit
    // is part of the production contract. This deadline only bounds the test.
    CHECK(close_returned.wait_for(timeout) == std::future_status::ready);
    closer.join();
    gate->check_still_blocked();
    CHECK(worker->snapshot().busy);
    check_closed(*worker);

    gate->release();
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    // busy becomes false under the same lock used for result publication.
    while (worker->snapshot().busy) {
        CHECK(std::chrono::steady_clock::now() < deadline);
        std::this_thread::yield();
    }
    check_closed(*worker); // Completion must not republish its stale result.
    gate.reset();
    worker.reset();
    CHECK(reclaimed.wait_for(timeout) == std::future_status::ready);
    CHECK(weak.expired());
}

void destruction_while_probe_is_blocked() {
    auto destroyed = std::make_shared<std::promise<void>>();
    auto reclaimed = destroyed->get_future();
    auto gate = std::make_shared<Gate>(destroyed);
    const std::weak_ptr<Gate> weak = gate;
    auto worker = std::make_unique<axv::AudioInventoryAsync>([gate] { return gate->probe(); });
    CHECK(worker->request() && worker->start_pending());
    gate->wait_entered();
    gate.reset(); // Only the worker's owned probe retains the dependency.

    std::promise<void> destroyed_worker;
    auto destructor_returned = destroyed_worker.get_future();
    std::thread destroyer([owned = std::move(worker), &destroyed_worker]() mutable {
        owned.reset();
        destroyed_worker.set_value();
    });
    CHECK(destructor_returned.wait_for(timeout) == std::future_status::ready);
    destroyer.join();
    CHECK(reclaimed.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
    {
        auto surviving_dependency = weak.lock();
        CHECK(surviving_dependency);
        surviving_dependency->check_still_blocked();
        surviving_dependency->release();
    }
    // Gate destruction proves that the detached worker finished and released
    // its State/Probe ownership after the public object ceased to exist.
    CHECK(reclaimed.wait_for(timeout) == std::future_status::ready);
    CHECK(weak.expired());
}
} // namespace

int main() {
    close_while_probe_is_blocked();
    destruction_while_probe_is_blocked();
}
