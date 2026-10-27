#include "../src/audio_device_change_signal.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
    std::abort(); \
} } while (false)

void coalesces_and_consumes_once() {
    axv::AudioDeviceChangeSignal signal;
    CHECK(!signal.take());
    for (int i = 0; i < 100; ++i) signal.notify();
    CHECK(signal.take());
    CHECK(!signal.take());
    signal.notify();
    CHECK(signal.take());
    CHECK(!signal.take());
}

void close_discards_pending_and_late_notifications() {
    axv::AudioDeviceChangeSignal signal;
    signal.notify();
    signal.close();
    CHECK(!signal.take());
    signal.close();
    for (int i = 0; i < 100; ++i) signal.notify();
    CHECK(!signal.take());
    signal.close();
    CHECK(!signal.take());
}

void concurrent_notifications_and_consumption() {
    axv::AudioDeviceChangeSignal signal;
    std::atomic<bool> start{false};
    std::atomic<int> remaining{4};
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&] {
            while (!start.load()) std::this_thread::yield();
            for (int n = 0; n < 10000; ++n) signal.notify();
            remaining.fetch_sub(1);
        });
    }
    start.store(true);
    bool observed = false;
    while (remaining.load() != 0) observed = signal.take() || observed;
    for (auto& producer : producers) producer.join();
    observed = signal.take() || observed;
    CHECK(observed);
    CHECK(!signal.take());
    signal.notify();
    CHECK(signal.take());
}

void closing_during_notifications_stays_closed() {
    auto signal = std::make_shared<axv::AudioDeviceChangeSignal>();
    std::atomic<bool> start{false};
    std::thread callback([signal, &start] {
        while (!start.load()) std::this_thread::yield();
        for (int i = 0; i < 100000; ++i) signal->notify();
    });
    start.store(true);
    signal->close();
    for (int i = 0; i < 10000; ++i) CHECK(!signal->take());
    callback.join();
    CHECK(!signal->take());
}

void restarted_observer_is_isolated() {
    auto old = std::make_shared<axv::AudioDeviceChangeSignal>();
    old->notify();
    old->close();
    auto current = std::make_shared<axv::AudioDeviceChangeSignal>();
    std::thread late_callback([old] { old->notify(); });
    late_callback.join();
    CHECK(!old->take());
    CHECK(!current->take());
    current->notify();
    CHECK(current->take());
    CHECK(!old->take());
    old->close();
    current->notify();
    CHECK(current->take());
}

int main() {
    coalesces_and_consumes_once();
    close_discards_pending_and_late_notifications();
    concurrent_notifications_and_consumption();
    closing_during_notifications_stays_closed();
    restarted_observer_is_isolated();
}
