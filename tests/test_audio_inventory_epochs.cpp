#include "../src/audio_inventory_async.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #x); std::abort(); } } while (false)
namespace {
using Clock = std::chrono::steady_clock;
constexpr auto timeout = std::chrono::seconds(5);
// Completion publication has no notification API; only this final transition is polled.
template<class Predicate> void await_publication(Predicate predicate) {
    const auto deadline = Clock::now() + timeout;
    while (!predicate()) {
        CHECK(Clock::now() < deadline);
        std::this_thread::yield();
    }
}
struct ProbeGate {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned calls = 0, active = 0, maximum_active = 0, released = 0;
    axv::AudioInventory probe() {
        std::unique_lock<std::mutex> lock(mutex);
        const auto call = ++calls;
        maximum_active = std::max(maximum_active, ++active);
        changed.notify_all();
        CHECK(changed.wait_until(lock, Clock::now() + timeout,
                                 [&] { return released >= call; }));
        --active;
        changed.notify_all();
        axv::AudioInventory result;
        result.input_count = call;
        result.output_count = 100 + call;
        return result;
    }
    void entered(unsigned count) {
        std::unique_lock<std::mutex> lock(mutex);
        CHECK(changed.wait_until(lock, Clock::now() + timeout,
                                 [&] { return calls >= count; }));
        CHECK(calls == count);
    }
    void release(unsigned count) {
        std::lock_guard<std::mutex> lock(mutex);
        released = count;
        changed.notify_all();
    }
    void check(unsigned count) {
        std::lock_guard<std::mutex> lock(mutex);
        CHECK(calls == count);
        CHECK(maximum_active == 1);
    }
};
void coalesced_requests() {
    auto gate = std::make_shared<ProbeGate>();
    axv::AudioInventoryAsync worker([gate] { return gate->probe(); });
    CHECK(worker.request());
    CHECK(worker.start_pending());
    gate->entered(1);
    std::vector<std::thread> requesters;
    for (unsigned i = 0; i < 8; ++i) {
        requesters.emplace_back([&] {
            for (unsigned j = 0; j < 8; ++j) {
                CHECK(worker.request());
                CHECK(!worker.start_pending());
            }
        });
    }
    for (auto& thread : requesters) thread.join();
    const auto queued = worker.snapshot();
    CHECK(queued.generation == 65);
    CHECK(queued.busy && queued.pending && !queued.ready);
    gate->check(1);
    gate->release(1);
    await_publication([&] { return !worker.snapshot().busy; });
    axv::AudioInventory result;
    CHECK(!worker.take(result));
    CHECK(worker.snapshot().pending && !worker.snapshot().ready);
    CHECK(worker.start_pending());
    gate->entered(2);
    CHECK(!worker.start_pending());
    CHECK(!worker.take(result));
    CHECK(worker.snapshot().generation == queued.generation);
    gate->release(2);
    await_publication([&] { return worker.snapshot().ready; });
    CHECK(worker.take(result));
    CHECK(result.input_count == 2 && result.output_count == 102);
    CHECK(!worker.take(result));
    CHECK(!worker.start_pending());
    CHECK(!worker.snapshot().pending && !worker.snapshot().busy);
    gate->check(2);
}
void cancelled_epoch(bool request_while_blocked) {
    auto gate = std::make_shared<ProbeGate>();
    axv::AudioInventoryAsync worker([gate] { return gate->probe(); });
    CHECK(worker.request() && worker.start_pending());
    gate->entered(1);
    worker.cancel();
    CHECK(worker.snapshot().generation == 2);
    CHECK(worker.snapshot().busy && !worker.snapshot().pending);
    CHECK(!worker.snapshot().ready);
    axv::AudioInventory result;
    CHECK(!worker.take(result));
    if (request_while_blocked) {
        CHECK(worker.request());
        CHECK(worker.snapshot().pending);
        CHECK(!worker.start_pending());
    }
    gate->release(1);
    await_publication([&] { return !worker.snapshot().busy; });
    CHECK(!worker.snapshot().ready && !worker.take(result));
    if (!request_while_blocked) {
        CHECK(!worker.start_pending());
        CHECK(worker.request());
    }
    CHECK(worker.snapshot().generation == 3);
    CHECK(worker.start_pending());
    gate->entered(2);
    gate->release(2);
    await_publication([&] { return worker.snapshot().ready; });
    CHECK(worker.take(result) && result.input_count == 2);
    CHECK(!worker.take(result) && !worker.start_pending());
    gate->check(2);
}
} // namespace
int main() {
    coalesced_requests();
    cancelled_epoch(false);
    cancelled_epoch(true);
}
