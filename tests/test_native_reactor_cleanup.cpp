#include "../src/baresip_native_libre_reactor.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>

// Real libre only: no Baresip initialization, UA, account, or network traffic.
// Explicit checks remain active in Release builds with NDEBUG.
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "cleanup check failed at line %d: %s\n", \
                 __LINE__, #condition); return 1; } } while (false)

int main() {
    using namespace std::chrono_literals;
    using axv::BaresipNativeLibreReactor;
    using axv::LibreReactorFault;
    const auto caller = std::this_thread::get_id();
    std::thread::id owner;
    bool sentinel = false;
    bool wrong_owner = false;
    unsigned starts = 0, stops = 0;
    BaresipNativeLibreReactor partial([] {}, [&] {
        owner = std::this_thread::get_id();
        ++starts;
        sentinel = true; // Models native state allocated before begin fails.
        throw std::runtime_error("partial initialization");
    }, [&] {
        wrong_owner |= std::this_thread::get_id() != owner || owner == caller;
        ++stops;
        sentinel = false;
    }, 5ms);
    for (unsigned attempt = 1; attempt <= 2; ++attempt) {
        CHECK(!partial.start());
        CHECK(partial.stop()); // Join before inspecting hook-owned state.
        CHECK(starts == attempt);
        CHECK(stops == attempt);
        CHECK(!sentinel && !wrong_owner);
        CHECK(!partial.snapshot().running);
        CHECK(partial.snapshot().fault == LibreReactorFault::HookFailed);
        CHECK(partial.snapshot().generation == 0);
        CHECK(!partial.wake());
        CHECK(partial.stop());
        CHECK(stops == attempt);
    }

    std::atomic<bool> tick_entered{false};
    stops = 0;
    BaresipNativeLibreReactor tick_failure([&] {
        wrong_owner |= std::this_thread::get_id() != owner;
        tick_entered = true;
        throw std::runtime_error("tick failure");
    }, [&] {
        owner = std::this_thread::get_id();
        sentinel = true;
    }, [&] {
        wrong_owner |= std::this_thread::get_id() != owner || owner == caller;
        ++stops;
        sentinel = false;
        throw std::runtime_error("stopping failure");
    }, 5ms);
    (void)tick_failure.start(); // The failed tick may finish before the caller wakes.
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!tick_entered && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(1ms);
    CHECK(tick_entered);
    CHECK(tick_failure.stop());
    CHECK(stops == 1 && !sentinel && !wrong_owner);
    CHECK(!tick_failure.snapshot().running);
    CHECK(tick_failure.snapshot().fault == LibreReactorFault::HookFailed);
    CHECK(!tick_failure.wake());
    CHECK(tick_failure.stop());
    CHECK(stops == 1);
    return 0;
}
