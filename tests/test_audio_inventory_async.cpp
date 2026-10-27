#include "../src/audio_inventory_async.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <stdexcept>

#define CHECK(x) do { if (!(x)) std::abort(); } while (false)
template<class Predicate> void wait_for(Predicate predicate) {
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!predicate()) {
        CHECK(std::chrono::steady_clock::now() < until);
        std::this_thread::yield();
    }
}
int main() {
    std::atomic<unsigned> calls{0};
    std::atomic<bool> release{false};
    axv::AudioInventoryAsync worker([&] {
        const auto call = ++calls;
        while (!release.load()) std::this_thread::yield();
        axv::AudioInventory result;
        result.input_count = call;
        return result;
    });
    CHECK(worker.snapshot().generation == 0);
    CHECK(worker.request());
    CHECK(calls == 0 && worker.snapshot().pending);
    CHECK(worker.start_pending());
    wait_for([&] { return calls == 1; });
    CHECK(worker.request());
    CHECK(worker.request());
    CHECK(!worker.start_pending());
    CHECK(worker.snapshot().generation == 3);
    release = true;
    wait_for([&] { return !worker.snapshot().busy; });
    axv::AudioInventory result;
    CHECK(!worker.take(result));
    CHECK(worker.snapshot().pending);
    CHECK(worker.start_pending());
    wait_for([&] { return worker.snapshot().ready; });
    CHECK(worker.take(result) && result.input_count == 2);
    CHECK(!worker.take(result));
    CHECK(worker.request());
    worker.cancel();
    CHECK(!worker.snapshot().pending && !worker.start_pending());
    CHECK(worker.request());
    CHECK(worker.start_pending());
    wait_for([&] { return worker.snapshot().ready; });
    worker.cancel();
    CHECK(!worker.take(result));
    CHECK(worker.request());
    CHECK(worker.start_pending());
    wait_for([&] { return worker.snapshot().ready; });
    CHECK(worker.request());
    CHECK(!worker.take(result));
    worker.close();
    CHECK(worker.snapshot().closed && !worker.request() && !worker.start_pending());

    axv::AudioInventoryAsync throwing([]() -> axv::AudioInventory { throw std::runtime_error("probe"); });
    CHECK(throwing.request() && throwing.start_pending());
    wait_for([&] { return throwing.snapshot().ready; });
    CHECK(throwing.take(result));
    CHECK(result.error == axv::AudioInventoryError::EndpointQueryFailed);

    CHECK(throwing.request());
    throwing.cancel();
    CHECK(!throwing.snapshot().pending && !throwing.take(result));

    struct Gate { std::atomic<bool> entered{false}, release{false}, done{false}; };
    auto gate = std::make_shared<Gate>();
    {
        axv::AudioInventoryAsync detached([gate] {
            gate->entered = true;
            while (!gate->release) std::this_thread::yield();
            gate->done = true;
            return axv::AudioInventory{};
        });
        CHECK(detached.request() && detached.start_pending());
        wait_for([&] { return gate->entered.load(); });
        detached.close();
        CHECK(detached.snapshot().closed && !detached.snapshot().pending);
    }
    gate->release = true;
    wait_for([&] { return gate->done.load(); });
}
