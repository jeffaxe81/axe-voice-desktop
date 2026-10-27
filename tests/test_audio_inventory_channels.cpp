#include "../src/audio_inventory_async.h"
#include "../src/audio_panel_selection.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>

#define CHECK(x) do { if (!(x)) std::abort(); } while (false)

namespace {
using namespace axv;
using Clock = std::chrono::steady_clock;
constexpr auto timeout = std::chrono::seconds(5);

struct ProbeGate {
    std::mutex mutex;
    std::condition_variable changed;
    unsigned calls = 0;
    bool release_first = false;

    unsigned enter(bool block_first) {
        std::unique_lock<std::mutex> lock(mutex);
        const unsigned call = ++calls;
        changed.notify_all();
        if (block_first && call == 1)
            CHECK(changed.wait_until(lock, Clock::now() + timeout,
                                    [&] { return release_first; }));
        return call;
    }
    void await_calls(unsigned count) {
        std::unique_lock<std::mutex> lock(mutex);
        CHECK(changed.wait_until(lock, Clock::now() + timeout,
                                [&] { return calls >= count; }));
    }
    void release() {
        std::lock_guard<std::mutex> lock(mutex);
        release_first = true;
        changed.notify_all();
    }
};

template<class Predicate> void await(Predicate predicate) {
    // The public async API exposes snapshots rather than a completion CV.
    // Bound observation waits without spinning or relying on probe timing.
    std::mutex mutex;
    std::condition_variable changed;
    std::unique_lock<std::mutex> lock(mutex);
    const auto deadline = Clock::now() + timeout;
    while (!predicate()) {
        CHECK(Clock::now() < deadline);
        changed.wait_until(lock, std::min(deadline, Clock::now() +
                                        std::chrono::milliseconds(1)));
    }
}

AudioInventory inventory(const std::wstring& prefix) {
    AudioInventory result;
    result.input_devices = {{prefix + L"-mic", L"Microphone", true, false}};
    result.output_devices = {{prefix + L"-call", L"Call", true, false},
                             {prefix + L"-ring", L"Ring", false, true}};
    result.input_count = 1;
    result.output_count = 2;
    return result;
}
bool equal(const AudioPanelSelection& left, const AudioPanelSelection& right) {
    return left.input_id == right.input_id && left.output_id == right.output_id &&
           left.ringtone_id == right.ringtone_id;
}
}

int main() {
    auto gate_a = std::make_shared<ProbeGate>();
    auto gate_b = std::make_shared<ProbeGate>();
    AudioInventoryAsync tab_a([gate_a] {
        const auto call = gate_a->enter(true);
        if (call == 2) throw std::runtime_error("query failed");
        return inventory(call == 1 ? L"A-stale" : L"A-recovered");
    });
    AudioInventoryAsync tab_b([gate_b] {
        gate_b->enter(false);
        return inventory(L"B");
    });
    CHECK(tab_a.request() && tab_a.start_pending());
    gate_a->await_calls(1);
    CHECK(tab_a.snapshot().busy);
    CHECK(tab_b.request() && tab_b.start_pending());
    gate_b->await_calls(1);
    await([&] { return tab_b.snapshot().ready; });
    CHECK(tab_a.snapshot().busy); // B completed while A's driver is blocked.

    // Navigate away from A, then return before its first query completes.
    const auto first_generation = tab_a.snapshot().generation;
    tab_a.cancel();
    CHECK(tab_a.request());
    CHECK(tab_a.snapshot().generation > first_generation);
    CHECK(tab_a.snapshot().pending && !tab_a.start_pending());
    CHECK(tab_b.snapshot().ready); // A cannot invalidate B's buffered result.
    AudioInventory a_result = inventory(L"sentinel-A");
    CHECK(!tab_a.take(a_result));
    CHECK(a_result.input_devices.front().id == L"sentinel-A-mic");
    AudioInventory b_result;
    CHECK(tab_b.take(b_result));
    CHECK(b_result.input_devices.front().id == L"B-mic");
    CHECK(!tab_b.take(b_result));
    const AudioPanelSelection b_choices = reconcile_audio_panel({}, b_result, false);
    CHECK(equal(b_choices, {L"B-mic", L"B-call", L"B-ring"}));

    gate_a->release();
    await([&] { return !tab_a.snapshot().busy; });
    CHECK(!tab_a.snapshot().ready && tab_a.snapshot().pending);
    CHECK(!tab_a.take(a_result)); // The stale pre-navigation result never appears.
    CHECK(!tab_b.take(b_result)); // B cannot receive A's discarded result either.
    CHECK(tab_a.start_pending());
    await([&] { return tab_a.snapshot().ready; });
    CHECK(tab_a.take(a_result));
    CHECK(a_result.error == AudioInventoryError::EndpointQueryFailed);
    const AudioPanelSelection a_choices{L"chosen-mic", L"chosen-call", L"chosen-ring"};
    const auto after_error = reconcile_audio_panel(a_choices, a_result, false);
    CHECK(equal(after_error, a_choices));
    CHECK(equal(reconcile_audio_panel(a_choices, a_result, true), a_choices));
    CHECK(equal(b_choices, {L"B-mic", L"B-call", L"B-ring"}));

    CHECK(tab_a.request() && tab_a.start_pending());
    await([&] { return tab_a.snapshot().ready; });
    CHECK(tab_a.take(a_result));
    CHECK(a_result.error == AudioInventoryError::None);
    // Native routes preserve unavailable explicit IDs after successful recovery.
    CHECK(equal(reconcile_audio_panel(after_error, a_result, true), a_choices));
    CHECK(equal(reconcile_audio_panel({}, a_result, true), {}));
    // Historical/default mode may fall back to communication/multimedia roles.
    CHECK(equal(reconcile_audio_panel(after_error, a_result, false),
                {L"A-recovered-mic", L"A-recovered-call", L"A-recovered-ring"}));
    CHECK(!tab_b.take(b_result)); // Even A's current result belongs only to A.
    CHECK(tab_b.request() && tab_b.start_pending());
    await([&] { return tab_b.snapshot().ready; });
    CHECK(tab_b.take(b_result));
    CHECK(equal(reconcile_audio_panel(b_choices, b_result, false), b_choices));
    CHECK(!tab_a.take(a_result) && !tab_b.take(b_result));
}
