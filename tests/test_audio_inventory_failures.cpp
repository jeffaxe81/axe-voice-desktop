#include "../src/audio_inventory_async.h"
#include "../src/audio_inventory_retry.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>

namespace { thread_local bool fail_next_allocation = false; }

// Fail only the calling thread's next allocation, after request publication.
void* operator new(std::size_t size) {
    if (fail_next_allocation) {
        fail_next_allocation = false;
        throw std::bad_alloc();
    }
    if (void* pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

#define CHECK(value) do { if (!(value)) { \
    std::fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #value); \
    std::abort(); \
} } while (false)

namespace {
void wait_until_ready(axv::AudioInventoryAsync& worker) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!worker.snapshot().ready && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    const auto status = worker.snapshot();
    CHECK(status.ready);
    CHECK(!status.busy);
}
}

int main() {
    {
        const auto calls = std::make_shared<std::atomic<unsigned>>(0);
        axv::AudioInventoryAsync worker([calls] {
            ++*calls;
            axv::AudioInventory result;
            result.input_count = 7;
            return result;
        });
        axv::AudioInventoryRetry retry;
        CHECK(worker.request());
        const auto generation = worker.snapshot().generation;
        fail_next_allocation = true;
        CHECK(!worker.start_pending());
        CHECK(!fail_next_allocation);
        const auto failed = worker.snapshot();
        CHECK(!failed.busy && failed.pending && failed.launch_failed);
        CHECK(!failed.ready && failed.generation == generation);
        CHECK(calls->load() == 0);
        axv::AudioInventory result;
        CHECK(!worker.take(result));
        constexpr std::uint64_t now = 4321;
        retry.failed(now);
        CHECK(!retry.ready(now));
        CHECK(!retry.ready(now + 999));
        CHECK(retry.ready(now + 1000));
        CHECK(worker.start_pending());
        retry.started();
        CHECK(retry.ready(0));
        const auto started = worker.snapshot();
        CHECK(!started.pending && !started.launch_failed);
        CHECK(started.generation == generation);
        wait_until_ready(worker);
        CHECK(worker.take(result));
        CHECK(result.error == axv::AudioInventoryError::None);
        CHECK(result.input_count == 7 && calls->load() == 1);
        CHECK(!worker.take(result));
        CHECK(!worker.start_pending());
        CHECK(!worker.snapshot().ready);
    }
    {
        const auto calls = std::make_shared<std::atomic<unsigned>>(0);
        axv::AudioInventoryAsync worker([calls] {
            if (++*calls == 1) throw 42;
            axv::AudioInventory result;
            result.output_count = 3;
            return result;
        });
        CHECK(worker.request());
        CHECK(worker.start_pending());
        wait_until_ready(worker);
        axv::AudioInventory result;
        CHECK(worker.take(result));
        CHECK(result.error == axv::AudioInventoryError::EndpointQueryFailed);
        CHECK(!worker.take(result));
        CHECK(calls->load() == 1);
        const auto prior_generation = worker.snapshot().generation;
        CHECK(worker.request());
        CHECK(worker.snapshot().generation == prior_generation + 1);
        CHECK(!worker.take(result));
        CHECK(worker.start_pending());
        wait_until_ready(worker);
        CHECK(worker.take(result));
        CHECK(result.error == axv::AudioInventoryError::None);
        CHECK(result.input_count == 0 && result.output_count == 3);
        CHECK(result.input_devices.empty() && result.output_devices.empty());
        CHECK(calls->load() == 2);
        CHECK(!worker.take(result));
        const auto recovered = worker.snapshot();
        CHECK(!recovered.busy && !recovered.pending && !recovered.ready);
        CHECK(!recovered.launch_failed);
    }
}
